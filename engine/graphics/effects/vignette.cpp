#include "graphics/effects/vignette.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

static const char* VIGNETTE_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_tex_coords;

out vec2 v_uv;

void main() {
    v_uv = a_tex_coords;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

static const char* VIGNETTE_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_texture;
uniform float u_intensity;
uniform float u_softness;

void main() {
    vec4 color = texture(u_texture, v_uv);
    float d = distance(v_uv, vec2(0.5));
    float vig = smoothstep(u_softness, u_softness - u_intensity, d);
    color.rgb *= vig;
    frag_color = color;
}
)";

bool VignetteEffect::init(i32 width, i32 height) {
    (void)width;
    (void)height;

    if (!m_shader.load_from_source(VIGNETTE_VERT, VIGNETTE_FRAG)) {
        log::error("vignette: failed to compile shader");
        return false;
    }

    setup_quad();

    log::info("vignette effect initialized");
    return true;
}

void VignetteEffect::shutdown() {
    if (m_quad_vao != 0) {
        glDeleteVertexArrays(1, &m_quad_vao);
        glDeleteBuffers(1, &m_quad_vbo);
        m_quad_vao = 0;
        m_quad_vbo = 0;
    }
}

void VignetteEffect::apply(u32 input_texture, Framebuffer& output) {
    output.bind();
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader.bind();
    m_shader.set_int("u_texture", 0);
    m_shader.set_float("u_intensity", m_intensity);
    m_shader.set_float("u_softness", m_softness);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);

    draw_quad();
}

void VignetteEffect::setup_quad() {
    float quad_verts[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_quad_vao);
    glGenBuffers(1, &m_quad_vbo);

    glBindVertexArray(m_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void VignetteEffect::draw_quad() {
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace kairo
