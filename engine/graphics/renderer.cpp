#include "graphics/renderer.h"
#include "graphics/batch_renderer.h"
#include "core/log.h"
#include "math/mat4.h"

#include <glad/glad.h>

namespace kairo {

// module-level state for the renderer
static struct RendererState {
    BatchRenderer batch;
    Shader shader;
    Renderer::Stats stats;
} s_state;

bool Renderer::init() {
    // enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // load the default shader
    if (!s_state.shader.load("assets/shaders/default.vert", "assets/shaders/default.frag")) {
        log::error("renderer: failed to load default shader");
        return false;
    }

    if (!s_state.batch.init()) {
        log::error("renderer: failed to initialize batch renderer");
        return false;
    }

    log::info("renderer initialized");
    return true;
}

void Renderer::shutdown() {
    s_state.batch.shutdown();
    log::info("renderer shut down");
}

void Renderer::begin(const Camera& camera) {
    s_state.shader.bind();
    s_state.shader.set_mat4("u_view_projection", camera.get_view_projection());
    s_state.batch.begin_batch();
}

void Renderer::end() {
    s_state.batch.end_batch();

    // accumulate stats
    s_state.stats.draw_calls += s_state.batch.get_draw_call_count();
    s_state.stats.quad_count += s_state.batch.get_quad_count();
    s_state.batch.reset_stats();
}

void Renderer::draw_quad(const Vec2& position, const Vec2& size, const Vec4& color) {
    draw_quad(Vec3(position.x, position.y, 0.0f), size, color);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, const Vec4& color) {
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;

    Vertex v0 = { { position.x - hw, position.y - hh, position.z }, color, { 0.0f, 0.0f } };
    Vertex v1 = { { position.x + hw, position.y - hh, position.z }, color, { 1.0f, 0.0f } };
    Vertex v2 = { { position.x + hw, position.y + hh, position.z }, color, { 1.0f, 1.0f } };
    Vertex v3 = { { position.x - hw, position.y + hh, position.z }, color, { 0.0f, 1.0f } };

    s_state.batch.submit_quad(v0, v1, v2, v3);
}

void Renderer::draw_quad(const Vec3& position, const Vec2& size, float rotation, const Vec4& color) {
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;

    // rotate corners around the quad center
    Mat4 transform = Mat4::translate(position)
                   * Mat4::rotate_z(rotation)
                   * Mat4::scale(Vec3(hw, hh, 1.0f));

    // unit quad corners
    Vec3 corners[4] = {
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
    };

    // transform each corner manually (mat4 * vec3 as position)
    Vertex verts[4];
    Vec2 uvs[4] = { {0,0}, {1,0}, {1,1}, {0,1} };
    for (int i = 0; i < 4; i++) {
        const float* m = transform.data();
        float x = corners[i].x, y = corners[i].y, z = corners[i].z;
        verts[i].position = {
            m[0]*x + m[4]*y + m[8]*z  + m[12],
            m[1]*x + m[5]*y + m[9]*z  + m[13],
            m[2]*x + m[6]*y + m[10]*z + m[14]
        };
        verts[i].color = color;
        verts[i].tex_coords = uvs[i];
    }

    s_state.batch.submit_quad(verts[0], verts[1], verts[2], verts[3]);
}

Renderer::Stats Renderer::get_stats() {
    return s_state.stats;
}

void Renderer::reset_stats() {
    s_state.stats = {};
}

} // namespace kairo
