#include "graphics/effects/bloom.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

// shared vertex shader for all bloom passes
static const char* BLOOM_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_tex_coords;

out vec2 v_uv;

void main() {
    v_uv = a_tex_coords;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

// brightness extraction — keep only pixels above threshold
static const char* EXTRACT_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_texture;
uniform float u_threshold;

void main() {
    vec3 color = texture(u_texture, v_uv).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_threshold) {
        frag_color = vec4(color, 1.0);
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";

// 5-tap gaussian blur (horizontal or vertical)
static const char* BLUR_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_texture;
uniform bool u_horizontal;

// 5-tap gaussian weights
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(u_texture, 0);
    vec3 result = texture(u_texture, v_uv).rgb * weights[0];

    if (u_horizontal) {
        for (int i = 1; i < 5; i++) {
            result += texture(u_texture, v_uv + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
            result += texture(u_texture, v_uv - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
        }
    } else {
        for (int i = 1; i < 5; i++) {
            result += texture(u_texture, v_uv + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
            result += texture(u_texture, v_uv - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
        }
    }

    frag_color = vec4(result, 1.0);
}
)";

// composite: add blurred bloom on top of original
static const char* COMPOSITE_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_texture;   // original scene
uniform sampler2D u_bloom;     // blurred bright areas
uniform float u_intensity;

void main() {
    vec3 scene = texture(u_texture, v_uv).rgb;
    vec3 bloom = texture(u_bloom, v_uv).rgb;
    frag_color = vec4(scene + bloom * u_intensity, 1.0);
}
)";

bool BloomEffect::init(i32 width, i32 height) {
    m_width  = width;
    m_height = height;

    i32 half_w = width / 2;
    i32 half_h = height / 2;

    // create half-res fbos
    FramebufferConfig cfg{half_w, half_h, false};
    if (!m_bright_fbo.init(cfg) || !m_blur_h_fbo.init(cfg) || !m_blur_v_fbo.init(cfg)) {
        log::error("bloom: failed to create internal fbos");
        return false;
    }

    // compile shaders
    if (!m_extract_shader.load_from_source(BLOOM_VERT, EXTRACT_FRAG)) {
        log::error("bloom: failed to compile extract shader");
        return false;
    }
    if (!m_blur_shader.load_from_source(BLOOM_VERT, BLUR_FRAG)) {
        log::error("bloom: failed to compile blur shader");
        return false;
    }
    if (!m_composite_shader.load_from_source(BLOOM_VERT, COMPOSITE_FRAG)) {
        log::error("bloom: failed to compile composite shader");
        return false;
    }

    setup_quad();

    log::info("bloom effect initialized (%dx%d, half-res %dx%d)",
              width, height, half_w, half_h);
    return true;
}

void BloomEffect::shutdown() {
    m_bright_fbo.shutdown();
    m_blur_h_fbo.shutdown();
    m_blur_v_fbo.shutdown();

    if (m_quad_vao != 0) {
        glDeleteVertexArrays(1, &m_quad_vao);
        glDeleteBuffers(1, &m_quad_vbo);
        m_quad_vao = 0;
        m_quad_vbo = 0;
    }
}

void BloomEffect::apply(u32 input_texture, Framebuffer& output) {
    // pass 1: extract bright pixels into half-res fbo
    m_bright_fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_extract_shader.bind();
    m_extract_shader.set_int("u_texture", 0);
    m_extract_shader.set_float("u_threshold", m_threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);
    draw_quad();

    // pass 2: horizontal blur
    m_blur_h_fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_blur_shader.bind();
    m_blur_shader.set_int("u_texture", 0);
    m_blur_shader.set_int("u_horizontal", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_bright_fbo.get_color_attachment());
    draw_quad();

    // pass 3: vertical blur
    m_blur_v_fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_blur_shader.bind();
    m_blur_shader.set_int("u_horizontal", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_blur_h_fbo.get_color_attachment());
    draw_quad();

    // pass 4: composite original + blurred bloom into output
    output.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_composite_shader.bind();
    m_composite_shader.set_int("u_texture", 0);
    m_composite_shader.set_int("u_bloom", 1);
    m_composite_shader.set_float("u_intensity", m_intensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blur_v_fbo.get_color_attachment());

    draw_quad();
}

void BloomEffect::setup_quad() {
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

void BloomEffect::draw_quad() {
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace kairo
