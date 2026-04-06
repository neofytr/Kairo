#include "physics/physics_world.h"
#include <algorithm>
#include <cmath>

namespace kairo {

void PhysicsWorld::step(std::vector<PhysicsBody>& bodies, float dt) {
    integrate(bodies, dt);
    detect_collisions(bodies);
    resolve_collisions(bodies);
}

void PhysicsWorld::integrate(std::vector<PhysicsBody>& bodies, float dt) {
    for (auto& body : bodies) {
        if (!body.rigidbody || body.rigidbody->is_static) continue;

        auto& rb = *body.rigidbody;

        // accumulate gravity + external forces
        Vec2 accel = m_gravity;
        if (rb.inv_mass > 0.0f) {
            accel += rb.force * rb.inv_mass;
        }

        rb.velocity += accel * dt;

        // light damping
        rb.velocity = rb.velocity * 0.999f;

        // integrate position
        body.position += rb.velocity * dt;

        // clear forces
        rb.force = { 0.0f, 0.0f };
    }
}

void PhysicsWorld::detect_collisions(const std::vector<PhysicsBody>& bodies) {
    m_collisions.clear();
    m_grid.clear();

    // build entity -> index lookup for narrowphase
    std::unordered_map<u64, size_t> entity_index;
    for (size_t i = 0; i < bodies.size(); i++) {
        AABB aabb = bodies[i].collider.get_aabb(bodies[i].position);
        m_grid.insert(bodies[i].entity, aabb);
        entity_index[bodies[i].entity.id] = i;
    }

    // broadphase: get candidate pairs from spatial grid
    auto pairs = m_grid.get_potential_pairs();

    // narrowphase: test each candidate pair (filtered by collision layers)
    for (auto& [e_a, e_b] : pairs) {
        size_t idx_a = entity_index[e_a.id];
        size_t idx_b = entity_index[e_b.id];

        // skip if collision layers don't allow interaction
        if (!bodies[idx_a].collider.can_collide_with(bodies[idx_b].collider)) continue;

        AABB aabb_a = bodies[idx_a].collider.get_aabb(bodies[idx_a].position);
        AABB aabb_b = bodies[idx_b].collider.get_aabb(bodies[idx_b].position);

        CollisionManifold manifold = aabb_vs_aabb(aabb_a, aabb_b);
        if (manifold.colliding) {
            m_collisions.push_back({ e_a, e_b, manifold });
        }
    }
}

void PhysicsWorld::resolve_collisions(std::vector<PhysicsBody>& bodies) {
    // build a quick lookup: entity -> body index
    // (could optimize with a flat map, fine for now)
    auto find_body = [&](Entity e) -> PhysicsBody* {
        for (auto& b : bodies) {
            if (b.entity == e) return &b;
        }
        return nullptr;
    };

    for (auto& col : m_collisions) {
        // fire callbacks
        for (auto& cb : m_callbacks) {
            cb(col.a, col.b, col.manifold);
        }

        PhysicsBody* body_a = find_body(col.a);
        PhysicsBody* body_b = find_body(col.b);
        if (!body_a || !body_b) continue;

        // skip if either is a trigger
        if (body_a->collider.is_trigger || body_b->collider.is_trigger) continue;

        RigidBodyComponent* rb_a = body_a->rigidbody;
        RigidBodyComponent* rb_b = body_b->rigidbody;

        float inv_mass_a = rb_a ? rb_a->inv_mass : 0.0f;
        float inv_mass_b = rb_b ? rb_b->inv_mass : 0.0f;
        float total_inv_mass = inv_mass_a + inv_mass_b;

        if (total_inv_mass <= 0.0f) continue;

        Vec2 normal = col.manifold.normal;
        float penetration = col.manifold.penetration;

        // relative velocity
        Vec2 vel_a = rb_a ? rb_a->velocity : Vec2{};
        Vec2 vel_b = rb_b ? rb_b->velocity : Vec2{};
        Vec2 relative_vel = vel_b - vel_a;
        float vel_along_normal = relative_vel.dot(normal);

        // skip if already separating
        if (vel_along_normal > 0.0f) continue;

        // coefficient of restitution
        float e = std::min(
            rb_a ? rb_a->restitution : 0.0f,
            rb_b ? rb_b->restitution : 0.0f
        );

        // impulse scalar
        float j = -(1.0f + e) * vel_along_normal / total_inv_mass;
        Vec2 impulse = normal * j;

        // apply impulse
        if (rb_a && !rb_a->is_static) {
            rb_a->velocity -= impulse * inv_mass_a;
        }
        if (rb_b && !rb_b->is_static) {
            rb_b->velocity += impulse * inv_mass_b;
        }

        // positional correction (prevent sinking)
        const float slop = 0.005f;
        const float correction_pct = 0.6f;
        float correction_mag = std::max(penetration - slop, 0.0f) / total_inv_mass * correction_pct;
        Vec2 correction = normal * correction_mag;

        if (rb_a && !rb_a->is_static) {
            body_a->position -= correction * inv_mass_a;
        }
        if (rb_b && !rb_b->is_static) {
            body_b->position += correction * inv_mass_b;
        }
    }
}

} // namespace kairo
