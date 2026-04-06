#include "gameplay/steering.h"
#include <cmath>
#include <cstdlib>

namespace kairo {
namespace steering {

// clamp vector to max length
Vec2 truncate(const Vec2& v, float max_length) {
    float len = v.length();
    if (len > max_length && len > 0.0f)
        return v * (max_length / len);
    return v;
}

// steer toward target
Vec2 seek(const SteeringAgent& agent, const Vec2& target) {
    Vec2 desired = (target - agent.position).normalized() * agent.max_speed;
    return truncate(desired - agent.velocity, agent.max_force);
}

// steer away from target
Vec2 flee(const SteeringAgent& agent, const Vec2& target) {
    Vec2 desired = (agent.position - target).normalized() * agent.max_speed;
    return truncate(desired - agent.velocity, agent.max_force);
}

// seek but decelerate within slow_radius
Vec2 arrive(const SteeringAgent& agent, const Vec2& target, float slow_radius) {
    Vec2 offset = target - agent.position;
    float dist = offset.length();
    if (dist < 0.0001f)
        return -agent.velocity;

    float speed = agent.max_speed;
    if (dist < slow_radius)
        speed *= dist / slow_radius;

    Vec2 desired = offset.normalized() * speed;
    return truncate(desired - agent.velocity, agent.max_force);
}

// seek predicted future position of moving target
Vec2 pursue(const SteeringAgent& agent, const Vec2& target_pos, const Vec2& target_vel) {
    float dist = (target_pos - agent.position).length();
    float prediction_time = dist / agent.max_speed;
    Vec2 predicted = target_pos + target_vel * prediction_time;
    return seek(agent, predicted);
}

// flee from predicted future position of moving target
Vec2 evade(const SteeringAgent& agent, const Vec2& target_pos, const Vec2& target_vel) {
    float dist = (target_pos - agent.position).length();
    float prediction_time = dist / agent.max_speed;
    Vec2 predicted = target_pos + target_vel * prediction_time;
    return flee(agent, predicted);
}

// random wandering via perturbed circle ahead of agent
Vec2 wander(const SteeringAgent& agent, float& wander_angle, float radius, float distance, float jitter) {
    // perturb wander angle by random amount in [-jitter, jitter]
    float rand_norm = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // [0,1]
    wander_angle += (rand_norm * 2.0f - 1.0f) * jitter;

    // circle center ahead of agent
    Vec2 heading = agent.velocity.length() > 0.0001f
        ? agent.velocity.normalized()
        : Vec2(1.0f, 0.0f);
    Vec2 circle_center = agent.position + heading * distance;

    // point on circle at wander_angle
    Vec2 displacement(std::cos(wander_angle) * radius, std::sin(wander_angle) * radius);
    Vec2 target = circle_center + displacement;

    return seek(agent, target);
}

// steer away from nearby neighbors
Vec2 separation(const SteeringAgent& agent, const std::vector<Vec2>& neighbors, float desired_distance) {
    Vec2 force;
    for (const auto& n : neighbors) {
        Vec2 away = agent.position - n;
        float dist = away.length();
        if (dist > 0.0001f && dist < desired_distance)
            force += away.normalized() / dist;
    }
    return truncate(force, agent.max_force);
}

// steer toward average position of neighbors
Vec2 cohesion(const SteeringAgent& agent, const std::vector<Vec2>& neighbors) {
    if (neighbors.empty())
        return {};

    Vec2 center;
    for (const auto& n : neighbors)
        center += n;
    center = center / static_cast<float>(neighbors.size());

    return seek(agent, center);
}

// match average velocity of neighbors
Vec2 alignment(const SteeringAgent& agent, const std::vector<Vec2>& neighbor_velocities) {
    if (neighbor_velocities.empty())
        return {};

    Vec2 avg;
    for (const auto& v : neighbor_velocities)
        avg += v;
    avg = avg / static_cast<float>(neighbor_velocities.size());

    return truncate(avg - agent.velocity, agent.max_force);
}

} // namespace steering
} // namespace kairo
