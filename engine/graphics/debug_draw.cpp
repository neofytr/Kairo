#include "graphics/debug_draw.h"
#include "graphics/shader.h"
#include "core/log.h"
#include "math/math_utils.h"

#include <glad/glad.h>
#include <cmath>

namespace kairo {

static const char* DEBUG_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_color;

uniform mat4 u_view_projection;

out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_view_projection * vec4(a_position, 0.0, 1.0);
}
)";

static const char* DEBUG_FRAG = R"(
#version 460 core
in vec4 v_color;
out vec4 frag_color;

void main() {
    frag_color = v_color;
}
)";

u32 DebugDraw::s_vao = 0;
u32 DebugDraw::s_vbo = 0;
Shader* DebugDraw::s_shader = nullptr;
bool DebugDraw::s_enabled = false;
DebugDraw::LineVertex* DebugDraw::s_buffer = nullptr;
u32 DebugDraw::s_vertex_count = 0;

bool DebugDraw::init() {
    s_shader = new Shader();
    if (!s_shader->load_from_source(DEBUG_VERT, DEBUG_FRAG)) {
        log::error("debug draw: failed to compile shader");
        return false;
    }

    s_buffer = new LineVertex[MAX_VERTICES];

    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
        (void*)offsetof(LineVertex, position));

    // color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
        (void*)offsetof(LineVertex, color));

    glBindVertexArray(0);

    log::info("debug draw initialized");
    return true;
}

void DebugDraw::shutdown() {
    delete s_shader;
    s_shader = nullptr;
    delete[] s_buffer;
    s_buffer = nullptr;
    glDeleteVertexArrays(1, &s_vao);
    glDeleteBuffers(1, &s_vbo);
}

void DebugDraw::line(const Vec2& a, const Vec2& b, const Vec4& color) {
    if (!s_enabled || s_vertex_count + 2 > MAX_VERTICES) return;
    s_buffer[s_vertex_count++] = { a, color };
    s_buffer[s_vertex_count++] = { b, color };
}

void DebugDraw::rect(const Vec2& center, const Vec2& size, const Vec4& color) {
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;
    Vec2 tl = { center.x - hw, center.y + hh };
    Vec2 tr = { center.x + hw, center.y + hh };
    Vec2 br = { center.x + hw, center.y - hh };
    Vec2 bl = { center.x - hw, center.y - hh };

    line(tl, tr, color);
    line(tr, br, color);
    line(br, bl, color);
    line(bl, tl, color);
}

void DebugDraw::circle(const Vec2& center, float radius, const Vec4& color, int segments) {
    float step = TAU / static_cast<float>(segments);
    for (int i = 0; i < segments; i++) {
        float a1 = step * i;
        float a2 = step * (i + 1);
        Vec2 p1 = { center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius };
        Vec2 p2 = { center.x + std::cos(a2) * radius, center.y + std::sin(a2) * radius };
        line(p1, p2, color);
    }
}

void DebugDraw::arrow(const Vec2& from, const Vec2& to, const Vec4& color) {
    line(from, to, color);

    // arrowhead
    Vec2 dir = (to - from).normalized();
    Vec2 perp = dir.perp();
    float head_len = 8.0f;
    Vec2 left  = to - dir * head_len + perp * head_len * 0.5f;
    Vec2 right = to - dir * head_len - perp * head_len * 0.5f;
    line(to, left, color);
    line(to, right, color);
}

void DebugDraw::render(const Camera& camera) {
    if (!s_enabled || s_vertex_count == 0) {
        s_vertex_count = 0;
        return;
    }

    // upload
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, s_vertex_count * sizeof(LineVertex), s_buffer);

    s_shader->bind();
    s_shader->set_mat4("u_view_projection", camera.get_view_projection());

    glBindVertexArray(s_vao);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, s_vertex_count);
    glBindVertexArray(0);

    s_vertex_count = 0;
}

} // namespace kairo
