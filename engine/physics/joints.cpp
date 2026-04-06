#include "physics/joints.h"
#include "physics/rigidbody.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace kairo {

void JointSystem::add_distance_joint(Entity a, Entity b, float rest_length,
                                     const Vec2& anchor_a, const Vec2& anchor_b) {
    Joint j;
    j.type = JointType::Distance;
    j.entity_a = a;
    j.entity_b = b;
    j.rest_length = rest_length;
    j.anchor_a = anchor_a;
    j.anchor_b = anchor_b;
    m_joints.push_back(j);
}

void JointSystem::add_spring_joint(Entity a, Entity b, float rest_length, float stiffness,
                                   float damping,
                                   const Vec2& anchor_a, const Vec2& anchor_b) {
    Joint j;
    j.type = JointType::Spring;
    j.entity_a = a;
    j.entity_b = b;
    j.rest_length = rest_length;
    j.stiffness = stiffness;
    j.damping = damping;
    j.anchor_a = anchor_a;
    j.anchor_b = anchor_b;
    m_joints.push_back(j);
}

void JointSystem::remove_joints(Entity entity) {
    m_joints.erase(
        std::remove_if(m_joints.begin(), m_joints.end(),
                       [&](const Joint& j) {
                           return j.entity_a == entity || j.entity_b == entity;
                       }),
        m_joints.end());
}

void JointSystem::solve(std::vector<BodyState>& bodies, float dt) {
    if (m_joints.empty() || bodies.empty()) return;

    // build entity -> index map for quick lookup
    std::unordered_map<Entity, size_t> index_map;
    index_map.reserve(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i) {
        index_map[bodies[i].entity] = i;
    }

    for (const auto& joint : m_joints) {
        auto it_a = index_map.find(joint.entity_a);
        auto it_b = index_map.find(joint.entity_b);
        if (it_a == index_map.end() || it_b == index_map.end()) continue;

        auto& state_a = bodies[it_a->second];
        auto& state_b = bodies[it_b->second];
        if (!state_a.rb || !state_b.rb) continue;

        switch (joint.type) {
            case JointType::Distance:
                solve_distance(joint, state_a, state_b, dt);
                break;
            case JointType::Spring:
                solve_spring(joint, state_a, state_b, dt);
                break;
        }
    }
}

void JointSystem::solve_distance(const Joint& j, BodyState& a, BodyState& b, float dt) {
    // world-space anchor positions
    Vec2 pa = a.position + j.anchor_a;
    Vec2 pb = b.position + j.anchor_b;

    Vec2 delta = pb - pa;
    float dist = delta.length();
    if (dist < 1e-6f) return; // overlapping, skip

    Vec2 dir = delta / dist;
    float error = dist - j.rest_length;

    // inverse mass sum for splitting correction
    float inv_mass_sum = a.rb->inv_mass + b.rb->inv_mass;
    if (inv_mass_sum < 1e-6f) return; // both static

    // baumgarte-style velocity correction
    float bias = 0.8f; // correction strength
    float correction = (bias / dt) * error;

    // apply as velocity impulse proportional to inverse mass
    Vec2 impulse = dir * (correction / inv_mass_sum);

    a.rb->velocity += impulse * a.rb->inv_mass;
    b.rb->velocity -= impulse * b.rb->inv_mass;
}

void JointSystem::solve_spring(const Joint& j, BodyState& a, BodyState& b, float dt) {
    // world-space anchor positions
    Vec2 pa = a.position + j.anchor_a;
    Vec2 pb = b.position + j.anchor_b;

    Vec2 delta = pb - pa;
    float dist = delta.length();
    if (dist < 1e-6f) return;

    Vec2 dir = delta / dist;

    // spring force: F = -k * (dist - rest)
    float stretch = dist - j.rest_length;
    float spring_force = -j.stiffness * stretch;

    // damping force along the spring axis
    Vec2 rel_vel = a.rb->velocity - b.rb->velocity;
    float vel_along = rel_vel.dot(dir);
    float damp_force = -j.damping * vel_along;

    // total scalar force (positive = push A away from B)
    float total = spring_force + damp_force;

    // clamp to max_force
    total = std::clamp(total, -j.max_force, j.max_force);

    Vec2 force = dir * total;

    // apply as velocity change: dv = F * dt * inv_mass
    a.rb->velocity += force * (dt * a.rb->inv_mass);
    b.rb->velocity -= force * (dt * b.rb->inv_mass);
}

void JointSystem::clear() {
    m_joints.clear();
}

size_t JointSystem::joint_count() const {
    return m_joints.size();
}

const std::vector<Joint>& JointSystem::get_joints() const {
    return m_joints;
}

} // namespace kairo
