#include "graphics/post_process.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

// passthrough blit shader
static const char* BLIT_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_tex_coords;

out vec2 v_uv;

void main() {
    v_uv = a_tex_coords;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

static const char* BLIT_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    frag_color = texture(u_texture, v_uv);
}
)";

bool PostProcessStack::init(i32 width, i32 height) {
    m_width  = width;
    m_height = height;

    // scene fbo with depth for rendering into
    FramebufferConfig scene_cfg{width, height, true};
    if (!m_scene_fbo.init(scene_cfg)) {
        log::error("post_process: failed to create scene fbo");
        return false;
    }

    // ping-pong fbos for chaining effects (no depth needed)
    FramebufferConfig pp_cfg{width, height, false};
    if (!m_ping.init(pp_cfg) || !m_pong.init(pp_cfg)) {
        log::error("post_process: failed to create ping/pong fbos");
        return false;
    }

    // blit shader
    if (!m_blit_shader.load_from_source(BLIT_VERT, BLIT_FRAG)) {
        log::error("post_process: failed to compile blit shader");
        return false;
    }

    // fullscreen quad (same layout as the light system)
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

    log::info("post-process stack initialized (%dx%d)", width, height);
    return true;
}

void PostProcessStack::shutdown() {
    for (auto& effect : m_effects) {
        effect->shutdown();
    }
    m_effects.clear();

    m_scene_fbo.shutdown();
    m_ping.shutdown();
    m_pong.shutdown();

    if (m_quad_vao != 0) {
        glDeleteVertexArrays(1, &m_quad_vao);
        glDeleteBuffers(1, &m_quad_vbo);
        m_quad_vao = 0;
        m_quad_vbo = 0;
    }
}

void PostProcessStack::add_effect(std::unique_ptr<PostProcessEffect> effect) {
    if (effect->init(m_width, m_height)) {
        log::info("post_process: added effect '%s'", effect->get_name());
        m_effects.push_back(std::move(effect));
    } else {
        log::error("post_process: failed to init effect '%s'", effect->get_name());
    }
}

void PostProcessStack::begin_capture() {
    m_scene_fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessStack::end_and_apply() {
    m_scene_fbo.unbind();

    if (m_effects.empty()) {
        // no effects — just blit scene to screen
        blit_to_screen(m_scene_fbo.get_color_attachment());
        return;
    }

    // chain effects: scene → effect[0] → ping, ping → effect[1] → pong, ...
    u32 current_input = m_scene_fbo.get_color_attachment();
    bool use_ping = true;

    for (size_t i = 0; i < m_effects.size(); ++i) {
        Framebuffer& target = use_ping ? m_ping : m_pong;
        m_effects[i]->apply(current_input, target);
        current_input = target.get_color_attachment();
        use_ping = !use_ping;
    }

    // blit final result to screen
    blit_to_screen(current_input);
}

void PostProcessStack::resize(i32 width, i32 height) {
    m_width  = width;
    m_height = height;

    m_scene_fbo.resize(width, height);
    m_ping.resize(width, height);
    m_pong.resize(width, height);

    for (auto& effect : m_effects) {
        effect->shutdown();
        effect->init(width, height);
    }
}

void PostProcessStack::draw_fullscreen_quad() {
    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessStack::blit_to_screen(u32 texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_blit_shader.bind();
    m_blit_shader.set_int("u_texture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    draw_fullscreen_quad();

    m_blit_shader.unbind();
}

} // namespace kairo
