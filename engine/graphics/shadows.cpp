#include "graphics/shadows.h"
#include "graphics/shader.h"
#include "graphics/camera.h"
#include "core/log.h"

#include <glad/glad.h>
#include <cmath>
#include <algorithm>

namespace kairo {

// simple shader: position -> solid dark color with configurable alpha
static const char* SHADOW_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;

uniform mat4 u_view_projection;

void main() {
    gl_Position = u_view_projection * vec4(a_position, 0.0, 1.0);
}
)";

static const char* SHADOW_FRAG = R"(
#version 460 core

out vec4 frag_color;

uniform float u_alpha;

void main() {
    frag_color = vec4(0.0, 0.0, 0.0, u_alpha);
}
)";

bool ShadowSystem::init() {
    m_shadow_shader = new Shader();
    if (!m_shadow_shader->load_from_source(SHADOW_VERT, SHADOW_FRAG)) {
        log::error("shadow system: failed to compile shadow shader");
        return false;
    }

    // dynamic VBO for shadow geometry, re-uploaded each frame
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    log::info("shadow system initialized");
    return true;
}

void ShadowSystem::shutdown() {
    if (m_shadow_shader) {
        delete m_shadow_shader;
        m_shadow_shader = nullptr;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        m_vao = 0;
        m_vbo = 0;
    }
}

void ShadowSystem::clear_casters() {
    m_casters.clear();
}

void ShadowSystem::add_caster(const ShadowCaster& caster) {
    m_casters.push_back(caster);
}

void ShadowSystem::compute_shadow_geometry(const Vec2& light_pos, const AABB& occluder,
                                           float light_radius, std::vector<float>& vertices) {
    // 4 corners of the AABB
    Vec2 corners[4] = {
        { occluder.min.x, occluder.min.y },  // bottom-left
        { occluder.max.x, occluder.min.y },  // bottom-right
        { occluder.max.x, occluder.max.y },  // top-right
        { occluder.min.x, occluder.max.y },  // top-left
    };

    // find the two silhouette corners — widest angular spread from the light
    // compute angle from light to each corner
    float angles[4];
    for (int i = 0; i < 4; i++) {
        Vec2 dir = corners[i] - light_pos;
        angles[i] = std::atan2(dir.y, dir.x);
    }

    // find the pair with maximum angular spread
    // we need to handle angle wrapping, so find min and max considering circular range
    int idx_a = 0, idx_b = 0;
    float max_spread = -1.0f;

    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            float diff = angles[j] - angles[i];
            // normalize to [-pi, pi]
            while (diff > 3.14159265f) diff -= 2.0f * 3.14159265f;
            while (diff < -3.14159265f) diff += 2.0f * 3.14159265f;
            float spread = std::abs(diff);
            if (spread > max_spread) {
                max_spread = spread;
                // ensure consistent winding: a is the "left" silhouette edge (CCW)
                if (diff > 0.0f) {
                    idx_a = i;
                    idx_b = j;
                } else {
                    idx_a = j;
                    idx_b = i;
                }
            }
        }
    }

    Vec2 ca = corners[idx_a];
    Vec2 cb = corners[idx_b];

    // project corners outward from the light
    Vec2 dir_a = (ca - light_pos).normalized();
    Vec2 dir_b = (cb - light_pos).normalized();

    Vec2 pa = ca + dir_a * light_radius;
    Vec2 pb = cb + dir_b * light_radius;

    // shadow quad: ca, cb, pb, pa — split into two triangles
    // triangle 1: ca, cb, pb
    vertices.push_back(ca.x); vertices.push_back(ca.y);
    vertices.push_back(cb.x); vertices.push_back(cb.y);
    vertices.push_back(pb.x); vertices.push_back(pb.y);

    // triangle 2: ca, pb, pa
    vertices.push_back(ca.x); vertices.push_back(ca.y);
    vertices.push_back(pb.x); vertices.push_back(pb.y);
    vertices.push_back(pa.x); vertices.push_back(pa.y);
}

void ShadowSystem::render_shadows(const Vec2& light_pos, float light_radius,
                                  const Camera& camera, float shadow_alpha) {
    if (m_casters.empty()) return;

    // build shadow geometry for all casters
    std::vector<float> vertices;
    vertices.reserve(m_casters.size() * 12); // 2 tris * 3 verts * 2 floats

    for (const auto& caster : m_casters) {
        compute_shadow_geometry(light_pos, caster.bounds, light_radius, vertices);
    }

    if (vertices.empty()) return;

    // upload to VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_DYNAMIC_DRAW);

    // render with alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shadow_shader->bind();
    m_shadow_shader->set_mat4("u_view_projection", camera.get_view_projection());
    m_shadow_shader->set_float("u_alpha", shadow_alpha);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));
    glBindVertexArray(0);
}

} // namespace kairo
