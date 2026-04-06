#pragma once

#include "particles/particle.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "graphics/camera.h"

#include <vector>
#include <random>

namespace kairo {

class ParticleEmitter {
public:
    ParticleEmitter() = default;
    explicit ParticleEmitter(const ParticleEmitterConfig& config);

    void init(const ParticleEmitterConfig& config);

    // set emitter world position
    void set_position(const Vec2& pos) { m_position = pos; }
    Vec2 get_position() const { return m_position; }

    // trigger a burst of particles
    void burst(i32 count);

    // update all particles
    void update(float dt);

    // render all alive particles
    void render(const Camera& camera);

    // control
    void start() { m_active = true; }
    void stop() { m_active = false; }
    bool is_active() const { return m_active; }

    u32 alive_count() const { return m_alive_count; }
    u32 max_particles() const { return m_config.max_particles; }

private:
    ParticleEmitterConfig m_config;
    std::vector<Particle> m_particles;
    Vec2 m_position = { 0, 0 };

    float m_emit_accumulator = 0.0f;
    bool m_active = true;
    u32 m_alive_count = 0;

    std::mt19937 m_rng{ std::random_device{}() };

    void emit(u32 count);
    void spawn_particle(Particle& p);

    // random helpers
    float rand_float(float lo, float hi);
    Vec2 rand_vec2(const Vec2& lo, const Vec2& hi);
    Vec4 rand_vec4(const Vec4& lo, const Vec4& hi);
};

} // namespace kairo
