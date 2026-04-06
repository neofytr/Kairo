#pragma once
#include "math/vec2.h"
#include <vector>

namespace kairo {

struct SteeringAgent {
    Vec2 position;
    Vec2 velocity;
    float max_speed = 100.0f;
    float max_force = 200.0f;
};

namespace steering {
    // basic behaviors — each returns a steering force vector
    Vec2 seek(const SteeringAgent& agent, const Vec2& target);
    Vec2 flee(const SteeringAgent& agent, const Vec2& target);
    Vec2 arrive(const SteeringAgent& agent, const Vec2& target, float slow_radius = 100.0f);
    Vec2 pursue(const SteeringAgent& agent, const Vec2& target_pos, const Vec2& target_vel);
    Vec2 evade(const SteeringAgent& agent, const Vec2& target_pos, const Vec2& target_vel);
    Vec2 wander(const SteeringAgent& agent, float& wander_angle, float radius = 50.0f, float distance = 80.0f, float jitter = 0.3f);

    // group behaviors
    Vec2 separation(const SteeringAgent& agent, const std::vector<Vec2>& neighbors, float desired_distance = 50.0f);
    Vec2 cohesion(const SteeringAgent& agent, const std::vector<Vec2>& neighbors);
    Vec2 alignment(const SteeringAgent& agent, const std::vector<Vec2>& neighbor_velocities);

    // utility
    Vec2 truncate(const Vec2& v, float max_length);
}

} // namespace kairo
