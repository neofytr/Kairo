#include "graphics/renderer.h"
#include "graphics/batch_renderer.h"
#include "core/log.h"
#include "math/mat4.h"

#include <glad/glad.h>
#include <vector>
#include <algorithm>

namespace kairo {

// a single queued draw command — sorted before submission
struct DrawCommand {
    Vertex verts[4];
    const Texture* texture; // nullptr = use white
    i32 layer;
    float z_depth;          // from position.z, for sorting within a layer
};

static struct RendererState {
    BatchRenderer batch;
    Shader shader;
    Texture white_texture;
    Renderer::Stats stats;

    i32 current_layer = static_cast<i32>(RenderLayer::Default);

    // deferred draw queue — sorted at end()
    std::vector<DrawCommand> draw_queue;
} s_state;

bool Renderer::init() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!s_state.shader.load("assets/shaders/default.vert", "assets/shaders/default.frag")) {
        log::error("renderer: failed to load default shader");
        return false;
    }

    if (!s_state.white_texture.create_white()) {
        log::error("renderer: failed to create white texture");
        return false;
    }

    if (!s_state.batch.init()) {
        log::error("renderer: failed to init batch renderer");
        return false;
    }

    s_state.batch.set_white_texture(s_state.white_texture.get_id());

    s_state.shader.bind();
    int samplers[16];
    for (int i = 0; i < 16; i++) samplers[i] = i;
    glUniform1iv(glGetUniformLocation(s_state.shader.get_id(), "u_textures"), 16, samplers);

    // reserve space for the draw queue to avoid per-frame allocations
    s_state.draw_queue.reserve(4096);

    log::info("renderer initialized (with layer sorting)");
    return true;
}

void Renderer::shutdown() {
    s_state.batch.shutdown();
    log::info("renderer shut down");
}

void Renderer::begin(const Camera& camera) {
    s_state.shader.bind();
    s_state.shader.set_mat4("u_view_projection", camera.get_view_projection());
    s_state.current_layer = static_cast<i32>(RenderLayer::Default);
    s_state.draw_queue.clear();
}

void Renderer::end() {
    // sort: layer first, then z-depth (back to front for correct transparency)
    std::sort(s_state.draw_queue.begin(), s_state.draw_queue.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            if (a.layer != b.layer) return a.layer < b.layer;
            return a.z_depth < b.z_depth; // lower z = further back = drawn first
        });

    // submit sorted commands to the batch renderer
    s_state.batch.begin_batch();
    for (auto& cmd : s_state.draw_queue) {
        s_state.batch.submit_textured_quad(
            cmd.verts[0], cmd.verts[1], cmd.verts[2], cmd.verts[3],
            cmd.texture);
    }
    s_state.batch.end_batch();

    s_state.stats.draw_calls += s_state.batch.get_draw_call_count();
    s_state.stats.quad_count += s_state.batch.get_quad_count();
    s_state.batch.reset_stats();
}

void Renderer::set_layer(RenderLayer layer) {
    s_state.current_layer = static_cast<i32>(layer);
}

void Renderer::set_layer(i32 layer) {
    s_state.current_layer = layer;
}

// helper to build vertices and enqueue a draw command
static void enqueue_quad(const Vec3& position, const Vec2& size, float rotation,
                         const Vec2& uv_min, const Vec2& uv_max,
                         const Vec4& color, const Texture* texture) {
    DrawCommand cmd;
    cmd.texture = texture;
    cmd.layer = s_state.current_layer;
    cmd.z_depth = position.z;

    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;

    Vec2 uvs[4] = {
        { uv_min.x, uv_min.y },
        { uv_max.x, uv_min.y },
        { uv_max.x, uv_max.y },
        { uv_min.x, uv_max.y },
    };

    if (rotation == 0.0f) {
        cmd.verts[0] = { { position.x - hw, position.y - hh, position.z }, color, uvs[0], 0.0f };
        cmd.verts[1] = { { position.x + hw, position.y - hh, position.z }, color, uvs[1], 0.0f };
        cmd.verts[2] = { { position.x + hw, position.y + hh, position.z }, color, uvs[2], 0.0f };
        cmd.verts[3] = { { position.x - hw, position.y + hh, position.z }, color, uvs[3], 0.0f };
    } else {
        Mat4 transform = Mat4::translate(position)
                       * Mat4::rotate_z(rotation)
                       * Mat4::scale(Vec3(hw, hh, 1.0f));

        Vec3 corners[4] = {
            { -1.0f, -1.0f, 0.0f },
            {  1.0f, -1.0f, 0.0f },
            {  1.0f,  1.0f, 0.0f },
            { -1.0f,  1.0f, 0.0f },
        };

        const float* m = transform.data();
        for (int i = 0; i < 4; i++) {
            float x = corners[i].x, y = corners[i].y, z = corners[i].z;
            cmd.verts[i].position = {
                m[0]*x + m[4]*y + m[8]*z  + m[12],
                m[1]*x + m[5]*y + m[9]*z  + m[13],
                m[2]*x + m[6]*y + m[10]*z + m[14]
            };
            cmd.verts[i].color = color;
            cmd.verts[i].tex_coords = uvs[i];
            cmd.verts[i].tex_index = 0.0f;
        }
    }

    s_state.draw_queue.push_back(cmd);
}

void Renderer::draw_quad(const Vec2& position, const Vec2& size, const Vec4& color) {
    draw_quad(Vec3(position.x, position.y, 0.0f), size, color);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, const Vec4& color) {
    enqueue_quad(position, size, 0.0f, {0,0}, {1,1}, color, nullptr);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, float rotation, const Vec4& color) {
    enqueue_quad(position, size, rotation, {0,0}, {1,1}, color, nullptr);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, const Texture& texture,
                          const Vec4& tint) {
    enqueue_quad(position, size, 0.0f, {0,0}, {1,1}, tint, &texture);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, float rotation,
                          const Texture& texture, const Vec4& tint) {
    enqueue_quad(position, size, rotation, {0,0}, {1,1}, tint, &texture);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, float rotation,
                          const Texture& texture, const Vec2& uv_min, const Vec2& uv_max,
                          const Vec4& tint) {
    enqueue_quad(position, size, rotation, uv_min, uv_max, tint, &texture);
}

Renderer::Stats Renderer::get_stats() {
    return s_state.stats;
}

void Renderer::reset_stats() {
    s_state.stats = {};
}

} // namespace kairo
