#include "physics/collision.h"

namespace kairo {

CollisionManifold test_collision(const ColliderComponent& a, const Vec2& pos_a,
                                 const ColliderComponent& b, const Vec2& pos_b) {
    using Shape = ColliderComponent::Shape;

    if (a.shape == Shape::AABB && b.shape == Shape::AABB) {
        return aabb_vs_aabb(a.get_aabb(pos_a), b.get_aabb(pos_b));
    }
    if (a.shape == Shape::Circle && b.shape == Shape::Circle) {
        return circle_vs_circle(a.get_circle(pos_a), b.get_circle(pos_b));
    }
    if (a.shape == Shape::Circle && b.shape == Shape::AABB) {
        return circle_vs_aabb(a.get_circle(pos_a), b.get_aabb(pos_b));
    }
    // a is AABB, b is circle
    return aabb_vs_circle(a.get_aabb(pos_a), b.get_circle(pos_b));
}

} // namespace kairo
