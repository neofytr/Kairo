#include "test_framework.h"
#include "ecs/hierarchy.h"
#include "ecs/tag_system.h"
#include "ecs/world.h"
#include "math/vec3.h"

using namespace kairo;

// ============================================================
// Tag System tests
// ============================================================

TEST(tag_and_has_tag) {
    World world;
    TagSystem ts;
    Entity e = world.create();

    ts.tag(world, e, "player");
    ASSERT_TRUE(ts.has_tag(world, e, "player"));
}

TEST(untag_removes_tag) {
    World world;
    TagSystem ts;
    Entity e = world.create();

    ts.tag(world, e, "enemy");
    ASSERT_TRUE(ts.has_tag(world, e, "enemy"));

    ts.untag(world, e, "enemy");
    ASSERT_FALSE(ts.has_tag(world, e, "enemy"));
}

TEST(has_tag_false_for_untagged) {
    World world;
    TagSystem ts;
    Entity e = world.create();

    // no tags added at all
    ASSERT_FALSE(ts.has_tag(world, e, "anything"));
}

TEST(find_by_tag_returns_correct_entities) {
    World world;
    TagSystem ts;
    Entity a = world.create();
    Entity b = world.create();
    Entity c = world.create();

    ts.tag(world, a, "enemy");
    ts.tag(world, b, "enemy");
    ts.tag(world, c, "player");

    auto enemies = ts.find_by_tag(world, "enemy");
    ASSERT_EQ(enemies.size(), (size_t)2);

    // both a and b should be in the result
    bool found_a = false, found_b = false;
    for (auto& e : enemies) {
        if (e == a) found_a = true;
        if (e == b) found_b = true;
    }
    ASSERT_TRUE(found_a);
    ASSERT_TRUE(found_b);
}

TEST(find_by_tag_empty_for_nonexistent) {
    World world;
    TagSystem ts;
    Entity e = world.create();
    ts.tag(world, e, "player");

    auto result = ts.find_by_tag(world, "nonexistent");
    ASSERT_EQ(result.size(), (size_t)0);
}

TEST(find_first_returns_match) {
    World world;
    TagSystem ts;
    Entity a = world.create();
    ts.tag(world, a, "boss");

    Entity found = ts.find_first(world, "boss");
    ASSERT_TRUE(found.is_valid());
    ASSERT_TRUE(found == a);
}

TEST(find_first_returns_null_when_none) {
    World world;
    TagSystem ts;
    Entity a = world.create();
    ts.tag(world, a, "player");

    Entity found = ts.find_first(world, "boss");
    ASSERT_TRUE(found == NULL_ENTITY);
}

TEST(multiple_tags_on_same_entity) {
    World world;
    TagSystem ts;
    Entity e = world.create();

    ts.tag(world, e, "player");
    ts.tag(world, e, "alive");
    ts.tag(world, e, "armed");

    ASSERT_TRUE(ts.has_tag(world, e, "player"));
    ASSERT_TRUE(ts.has_tag(world, e, "alive"));
    ASSERT_TRUE(ts.has_tag(world, e, "armed"));

    // removing one should not affect others
    ts.untag(world, e, "alive");
    ASSERT_FALSE(ts.has_tag(world, e, "alive"));
    ASSERT_TRUE(ts.has_tag(world, e, "player"));
    ASSERT_TRUE(ts.has_tag(world, e, "armed"));
}

// ============================================================
// Hierarchy tests
// ============================================================

TEST(hierarchy_attach_sets_parent) {
    World world;
    HierarchySystem hs;
    Entity parent = world.create();
    Entity child = world.create();

    hs.attach(world, child, parent);
    ASSERT_TRUE(hs.get_parent(world, child) == parent);
}

TEST(hierarchy_attach_adds_to_children_list) {
    World world;
    HierarchySystem hs;
    Entity parent = world.create();
    Entity child = world.create();

    hs.attach(world, child, parent);
    const auto& children = hs.get_children(world, parent);
    ASSERT_EQ(children.size(), (size_t)1);
    ASSERT_TRUE(children[0] == child);
}

TEST(hierarchy_detach_removes_parent) {
    World world;
    HierarchySystem hs;
    Entity parent = world.create();
    Entity child = world.create();

    hs.attach(world, child, parent);
    hs.detach(world, child);
    ASSERT_TRUE(hs.get_parent(world, child) == NULL_ENTITY);
}

TEST(hierarchy_detach_removes_from_children) {
    World world;
    HierarchySystem hs;
    Entity parent = world.create();
    Entity child = world.create();

    hs.attach(world, child, parent);
    hs.detach(world, child);
    const auto& children = hs.get_children(world, parent);
    ASSERT_EQ(children.size(), (size_t)0);
}

TEST(hierarchy_get_parent_null_for_root) {
    World world;
    HierarchySystem hs;
    Entity root = world.create();

    // no parent attached
    ASSERT_TRUE(hs.get_parent(world, root) == NULL_ENTITY);
}

TEST(hierarchy_get_children_empty_for_leaf) {
    World world;
    HierarchySystem hs;
    Entity leaf = world.create();

    const auto& children = hs.get_children(world, leaf);
    ASSERT_EQ(children.size(), (size_t)0);
}

TEST(hierarchy_world_pos_no_parent_returns_local) {
    World world;
    HierarchySystem hs;
    Entity e = world.create();

    std::unordered_map<u64, Vec3> positions;
    positions[e.id] = Vec3(10.0f, 20.0f, 30.0f);

    Vec3 wp = hs.compute_world_position(world, e, positions);
    ASSERT_NEAR(wp.x, 10.0f, 1e-4f);
    ASSERT_NEAR(wp.y, 20.0f, 1e-4f);
    ASSERT_NEAR(wp.z, 30.0f, 1e-4f);
}

TEST(hierarchy_world_pos_accumulates_parents) {
    World world;
    HierarchySystem hs;
    Entity grandparent = world.create();
    Entity parent = world.create();
    Entity child = world.create();

    hs.attach(world, parent, grandparent);
    hs.attach(world, child, parent);

    std::unordered_map<u64, Vec3> positions;
    positions[grandparent.id] = Vec3(100.0f, 0.0f, 0.0f);
    positions[parent.id] = Vec3(10.0f, 0.0f, 0.0f);
    positions[child.id] = Vec3(1.0f, 0.0f, 0.0f);

    // child world pos = 1 + 10 + 100 = 111
    Vec3 wp = hs.compute_world_position(world, child, positions);
    ASSERT_NEAR(wp.x, 111.0f, 1e-4f);
    ASSERT_NEAR(wp.y, 0.0f, 1e-4f);
    ASSERT_NEAR(wp.z, 0.0f, 1e-4f);
}
