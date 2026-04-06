#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include <vector>

namespace kairo {

struct PointLight {
    Vec2 position = { 0, 0 };
    Vec3 color = { 1, 1, 1 };
    float intensity = 1.0f;
    float radius = 200.0f;  // falloff distance
};

// manages 2D lights and renders a light map
// the light map is multiplied with the scene to produce the final image
class LightSystem {
public:
    static constexpr u32 MAX_LIGHTS = 32; // match the shader

    bool init();
    void shutdown();

    void set_ambient(const Vec3& color, float intensity = 0.1f);

    void clear_lights();
    void add_light(const PointLight& light);

    // render the lighting pass — call after scene render, before swap
    // this renders a fullscreen quad that multiplies the light contribution
    void render(const struct Camera& camera);

    const Vec3& get_ambient_color() const { return m_ambient_color; }
    float get_ambient_intensity() const { return m_ambient_intensity; }
    u32 light_count() const { return static_cast<u32>(m_lights.size()); }

private:
    class Shader* m_light_shader = nullptr;
    u32 m_quad_vao = 0;
    u32 m_quad_vbo = 0;

    Vec3 m_ambient_color = { 0.1f, 0.1f, 0.15f };
    float m_ambient_intensity = 0.15f;

    std::vector<PointLight> m_lights;
};

} // namespace kairo
