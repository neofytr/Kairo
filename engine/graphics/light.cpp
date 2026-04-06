#include "graphics/light.h"
#include "graphics/shader.h"
#include "graphics/camera.h"
#include "core/log.h"

#include <glad/glad.h>
#include <algorithm>

namespace kairo {

// the light shader works entirely in world space
// we pass camera position and ortho half-size so the fragment shader
// can reconstruct world position from screen UV
static const char* LIGHT_VERT = R"(
#version 460 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_tex_coords;

out vec2 v_uv;

void main() {
    v_uv = a_tex_coords;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

static const char* LIGHT_FRAG = R"(
#version 460 core

in vec2 v_uv;
out vec4 frag_color;

// camera info for reconstructing world position
uniform vec2 u_camera_pos;
uniform vec2 u_camera_half_size;

uniform vec3 u_ambient_color;
uniform float u_ambient_intensity;

uniform int u_light_count;
uniform vec2 u_light_positions[32];
uniform vec3 u_light_colors[32];
uniform float u_light_intensities[32];
uniform float u_light_radii[32];

void main() {
    // reconstruct world position from screen UV
    vec2 world_pos = u_camera_pos + (v_uv - 0.5) * u_camera_half_size * 2.0;

    vec3 total_light = u_ambient_color * u_ambient_intensity;

    for (int i = 0; i < u_light_count; i++) {
        float dist = distance(world_pos, u_light_positions[i]);
        float attenuation = 1.0 - smoothstep(0.0, u_light_radii[i], dist);
        attenuation *= attenuation; // quadratic falloff feels more natural
        total_light += u_light_colors[i] * u_light_intensities[i] * attenuation;
    }

    frag_color = vec4(total_light, 1.0);
}
)";

bool LightSystem::init() {
    m_light_shader = new Shader();
    if (!m_light_shader->load_from_source(LIGHT_VERT, LIGHT_FRAG)) {
        log::error("light system: failed to compile light shader");
        return false;
    }

    // fullscreen quad in NDC
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    log::info("light system initialized (max %d lights)", MAX_LIGHTS);
    return true;
}

void LightSystem::shutdown() {
    if (m_light_shader) {
        delete m_light_shader;
        m_light_shader = nullptr;
    }
    if (m_quad_vao) {
        glDeleteVertexArrays(1, &m_quad_vao);
        glDeleteBuffers(1, &m_quad_vbo);
        m_quad_vao = 0;
        m_quad_vbo = 0;
    }
}

void LightSystem::set_ambient(const Vec3& color, float intensity) {
    m_ambient_color = color;
    m_ambient_intensity = intensity;
}

void LightSystem::clear_lights() {
    m_lights.clear();
}

void LightSystem::add_light(const PointLight& light) {
    if (m_lights.size() < MAX_LIGHTS) {
        m_lights.push_back(light);
    }
}

void LightSystem::render(const Camera& camera) {
    // multiply blending: final = scene_color * light_color
    glBlendFunc(GL_DST_COLOR, GL_ZERO);

    m_light_shader->bind();

    // pass camera info for world position reconstruction
    auto cam_pos = camera.get_position();
    glUniform2f(glGetUniformLocation(m_light_shader->get_id(), "u_camera_pos"),
                cam_pos.x, cam_pos.y);
    // half size of the ortho view — hardcoded for now, should come from camera
    glUniform2f(glGetUniformLocation(m_light_shader->get_id(), "u_camera_half_size"),
                640.0f, 360.0f);

    m_light_shader->set_vec3("u_ambient_color", m_ambient_color);
    m_light_shader->set_float("u_ambient_intensity", m_ambient_intensity);

    int count = static_cast<int>(std::min((size_t)MAX_LIGHTS, m_lights.size()));
    m_light_shader->set_int("u_light_count", count);

    for (int i = 0; i < count; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "u_light_positions[%d]", i);
        glUniform2f(glGetUniformLocation(m_light_shader->get_id(), buf),
                    m_lights[i].position.x, m_lights[i].position.y);

        snprintf(buf, sizeof(buf), "u_light_colors[%d]", i);
        m_light_shader->set_vec3(buf, m_lights[i].color);

        snprintf(buf, sizeof(buf), "u_light_intensities[%d]", i);
        m_light_shader->set_float(buf, m_lights[i].intensity);

        snprintf(buf, sizeof(buf), "u_light_radii[%d]", i);
        m_light_shader->set_float(buf, m_lights[i].radius);
    }

    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // restore normal alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace kairo
