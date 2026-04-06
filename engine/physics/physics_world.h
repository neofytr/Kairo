#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "physics/collision.h"
#include "physics/rigidbody.h"
#include "physics/aabb.h"
#include "physics/spatial_grid.h"
#include "math/vec2.h"

#include <vector>
#include <functional>

namespace kairo {

class World;

using CollisionCallback = std::function<void(Entity a, Entity b, const CollisionManifold& manifold)>;

// physics body — a snapshot of an entity's physical state for the physics step
// the game fills these in from its TransformComponent + ColliderComponent + RigidBodyComponent
struct PhysicsBody {
    Entity entity;
    Vec2 position;
    ColliderComponent collider;
    RigidBodyComponent* rigidbody = nullptr; // pointer into ECS, null if no rigidbody
};

class PhysicsWorld {
public:
    void set_gravity(const Vec2& gravity) { m_gravity = gravity; }
    Vec2 get_gravity() const { return m_gravity; }

    // the game calls this each fixed step:
    // 1. pass in a list of physics bodies (collected from ECS by the game)
    // 2. physics world detects collisions and resolves them
    // 3. game reads back modified velocities and applies to transforms
    void step(std::vector<PhysicsBody>& bodies, float dt);

    void on_collision(CollisionCallback callback) { m_callbacks.push_back(std::move(callback)); }
    const std::vector<CollisionResult>& get_collisions() const { return m_collisions; }

private:
    Vec2 m_gravity = { 0.0f, 0.0f };
    SpatialGrid m_grid{ 100.0f };
    std::vector<CollisionResult> m_collisions;
    std::vector<CollisionCallback> m_callbacks;

    void integrate(std::vector<PhysicsBody>& bodies, float dt);
    void detect_collisions(const std::vector<PhysicsBody>& bodies);
    void resolve_collisions(std::vector<PhysicsBody>& bodies);
};

} // namespace kairo
