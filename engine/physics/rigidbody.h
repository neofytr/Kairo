#pragma once

#include "math/vec2.h"

namespace kairo {

struct RigidBodyComponent {
    Vec2 velocity = { 0.0f, 0.0f };
    Vec2 acceleration = { 0.0f, 0.0f };
    Vec2 force = { 0.0f, 0.0f }; // accumulated force, cleared each step

    float mass = 1.0f;
    float inv_mass = 1.0f;       // precomputed 1/mass, 0 = infinite mass (static)
    float restitution = 0.3f;    // bounciness (0 = no bounce, 1 = perfect bounce)
    float friction = 0.2f;

    bool is_static = false;      // static bodies don't move

    void set_mass(float m) {
        mass = m;
        inv_mass = (m > 0.0f) ? 1.0f / m : 0.0f;
    }

    void make_static() {
        is_static = true;
        mass = 0.0f;
        inv_mass = 0.0f;
        velocity = { 0.0f, 0.0f };
    }

    void apply_force(const Vec2& f) {
        force += f;
    }
};

} // namespace kairo
