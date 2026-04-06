#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec4.h"

#include <random>

namespace kairo {

// range type for randomized particle properties
template<typename T>
struct Range {
    T min{};
    T max{};

    Range() = default;
    Range(T value) : min(value), max(value) {}
    Range(T min, T max) : min(min), max(max) {}
};

// how a particle emitter spawns particles
struct ParticleEmitterConfig {
    // spawn
    u32 max_particles = 1000;
    float emit_rate = 50.0f;          // particles per second
    i32 burst_count = 0;              // if > 0, emit this many at once then stop

    // lifetime
    Range<float> lifetime = { 0.5f, 2.0f };

    // position offset from emitter
    Range<Vec2> offset = { {0, 0}, {0, 0} };

    // initial velocity
    Range<Vec2> velocity = { {-50, -50}, {50, 50} };

    // acceleration (gravity, wind, etc.)
    Vec2 acceleration = { 0.0f, 0.0f };

    // size over lifetime
    Range<float> start_size = { 4.0f, 8.0f };
    Range<float> end_size = { 0.0f, 1.0f };

    // color over lifetime
    Range<Vec4> start_color = { {1,1,1,1}, {1,1,1,1} };
    Range<Vec4> end_color = { {1,1,1,0}, {1,1,1,0} };

    // rotation
    Range<float> start_rotation = { 0.0f, 0.0f };
    Range<float> rotation_speed = { 0.0f, 0.0f };

    // damping applied to velocity each second (0 = none, 1 = full stop)
    float damping = 0.0f;

    bool looping = true;
};

// a single particle's runtime state — stored in SoA arrays, not individually
struct Particle {
    Vec2 position;
    Vec2 velocity;
    Vec4 start_color;
    Vec4 end_color;
    float start_size;
    float end_size;
    float rotation;
    float rotation_speed;
    float lifetime;      // total lifetime
    float elapsed;       // time alive
    bool alive;
};

} // namespace kairo
