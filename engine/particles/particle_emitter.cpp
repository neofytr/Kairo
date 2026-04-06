#include "particles/particle_emitter.h"
#include "graphics/renderer.h"
#include "math/math_utils.h"

#include <algorithm>

namespace kairo {

ParticleEmitter::ParticleEmitter(const ParticleEmitterConfig& config) {
    init(config);
}

void ParticleEmitter::init(const ParticleEmitterConfig& config) {
    m_config = config;
    m_particles.resize(config.max_particles);

    for (auto& p : m_particles) {
        p.alive = false;
    }

    m_alive_count = 0;
}

void ParticleEmitter::burst(i32 count) {
    emit(static_cast<u32>(count));
}

void ParticleEmitter::update(float dt) {
    // emit new particles based on rate
    if (m_active && m_config.looping && m_config.emit_rate > 0.0f) {
        m_emit_accumulator += m_config.emit_rate * dt;
        u32 to_emit = static_cast<u32>(m_emit_accumulator);
        if (to_emit > 0) {
            emit(to_emit);
            m_emit_accumulator -= static_cast<float>(to_emit);
        }
    }

    // update alive particles
    m_alive_count = 0;
    for (auto& p : m_particles) {
        if (!p.alive) continue;

        p.elapsed += dt;
        if (p.elapsed >= p.lifetime) {
            p.alive = false;
            continue;
        }

        // integrate velocity
        p.velocity += m_config.acceleration * dt;

        // damping
        if (m_config.damping > 0.0f) {
            float factor = 1.0f - m_config.damping * dt;
            if (factor < 0.0f) factor = 0.0f;
            p.velocity = p.velocity * factor;
        }

        p.position += p.velocity * dt;
        p.rotation += p.rotation_speed * dt;

        m_alive_count++;
    }
}

void ParticleEmitter::render(const Camera& camera) {
    for (auto& p : m_particles) {
        if (!p.alive) continue;

        float t = p.elapsed / p.lifetime;

        // interpolate size and color
        float size = lerp(p.start_size, p.end_size, t);
        Vec4 color = {
            lerp(p.start_color.x, p.end_color.x, t),
            lerp(p.start_color.y, p.end_color.y, t),
            lerp(p.start_color.z, p.end_color.z, t),
            lerp(p.start_color.w, p.end_color.w, t),
        };

        Renderer::draw_quad(
            Vec3(p.position.x, p.position.y, 0.0f),
            Vec2(size, size),
            p.rotation,
            color
        );
    }
}

void ParticleEmitter::emit(u32 count) {
    u32 spawned = 0;
    for (auto& p : m_particles) {
        if (spawned >= count) break;
        if (p.alive) continue;

        spawn_particle(p);
        spawned++;
    }
}

void ParticleEmitter::spawn_particle(Particle& p) {
    p.alive = true;
    p.elapsed = 0.0f;
    p.lifetime = rand_float(m_config.lifetime.min, m_config.lifetime.max);

    Vec2 offset = rand_vec2(m_config.offset.min, m_config.offset.max);
    p.position = m_position + offset;

    p.velocity = rand_vec2(m_config.velocity.min, m_config.velocity.max);

    p.start_size = rand_float(m_config.start_size.min, m_config.start_size.max);
    p.end_size = rand_float(m_config.end_size.min, m_config.end_size.max);

    p.start_color = rand_vec4(m_config.start_color.min, m_config.start_color.max);
    p.end_color = rand_vec4(m_config.end_color.min, m_config.end_color.max);

    p.rotation = rand_float(m_config.start_rotation.min, m_config.start_rotation.max);
    p.rotation_speed = rand_float(m_config.rotation_speed.min, m_config.rotation_speed.max);
}

float ParticleEmitter::rand_float(float lo, float hi) {
    if (lo >= hi) return lo;
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(m_rng);
}

Vec2 ParticleEmitter::rand_vec2(const Vec2& lo, const Vec2& hi) {
    return { rand_float(lo.x, hi.x), rand_float(lo.y, hi.y) };
}

Vec4 ParticleEmitter::rand_vec4(const Vec4& lo, const Vec4& hi) {
    return { rand_float(lo.x, hi.x), rand_float(lo.y, hi.y),
             rand_float(lo.z, hi.z), rand_float(lo.w, hi.w) };
}

} // namespace kairo
