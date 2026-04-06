#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "ecs/entity.h"
#include <vector>

namespace kairo {

struct RigidBodyComponent; // forward declare

enum class JointType {
    Distance, // maintains fixed distance between two points
    Spring,   // elastic connection with stiffness and damping
};

struct Joint {
    JointType type;
    Entity entity_a;
    Entity entity_b;

    Vec2 anchor_a = {0, 0}; // local offset on body A
    Vec2 anchor_b = {0, 0}; // local offset on body B

    float rest_length = 100.0f;  // target distance
    float stiffness = 50.0f;     // spring constant (for Spring type)
    float damping = 5.0f;        // damping coefficient
    float max_force = 10000.0f;  // prevent explosion
};

class JointSystem {
public:
    // add a distance joint between two entities
    void add_distance_joint(Entity a, Entity b, float rest_length,
                            const Vec2& anchor_a = {0, 0}, const Vec2& anchor_b = {0, 0});

    // add a spring joint
    void add_spring_joint(Entity a, Entity b, float rest_length, float stiffness = 50.0f,
                          float damping = 5.0f,
                          const Vec2& anchor_a = {0, 0}, const Vec2& anchor_b = {0, 0});

    // remove all joints involving an entity
    void remove_joints(Entity entity);

    // solve all joints -- modifies velocities of connected bodies
    // call this during the physics step
    struct BodyState {
        Entity entity;
        Vec2 position;
        RigidBodyComponent* rb;
    };
    void solve(std::vector<BodyState>& bodies, float dt);

    void clear();
    size_t joint_count() const;
    const std::vector<Joint>& get_joints() const;

private:
    std::vector<Joint> m_joints;

    void solve_distance(const Joint& j, BodyState& a, BodyState& b, float dt);
    void solve_spring(const Joint& j, BodyState& a, BodyState& b, float dt);
};

} // namespace kairo
