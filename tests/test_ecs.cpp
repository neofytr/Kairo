#include "test_framework.h"
#include "ecs/world.h"
#include "ecs/entity.h"
#include "ecs/component.h"

using namespace kairo;

// ---- test components (local to this file) ----
struct Position { float x = 0, y = 0; };
struct Velocity { float vx = 0, vy = 0; };
struct Health { int hp = 100; };
struct Tag {};  // zero-size component

// =====================================================================
// Entity tests
// =====================================================================

TEST(entity_make_creates_correct_id) {
    Entity e = Entity::make(42, 7);
    ASSERT_EQ(e.index(), 42u);
    ASSERT_EQ(e.generation(), 7u);
}

TEST(entity_index_and_generation_extract) {
    Entity e = Entity::make(0xDEAD, 0xBEEF);
    ASSERT_EQ(e.index(), 0xDEADu);
    ASSERT_EQ(e.generation(), 0xBEEFu);
}

TEST(entity_null_is_not_valid) {
    ASSERT_FALSE(NULL_ENTITY.is_valid());
    ASSERT_EQ(NULL_ENTITY.id, 0u);
}

TEST(entity_equality) {
    Entity a = Entity::make(1, 1);
    Entity b = Entity::make(1, 1);
    Entity c = Entity::make(1, 2);
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a == c);
    ASSERT_TRUE(a != c);
    ASSERT_FALSE(a != b);
}

// =====================================================================
// World basic tests
// =====================================================================

TEST(world_create_returns_valid_entity) {
    World w;
    Entity e = w.create();
    ASSERT_TRUE(e.is_valid());
}

TEST(world_is_alive_true_for_living) {
    World w;
    Entity e = w.create();
    ASSERT_TRUE(w.is_alive(e));
}

TEST(world_destroy_makes_not_alive) {
    World w;
    Entity e = w.create();
    w.destroy(e);
    ASSERT_FALSE(w.is_alive(e));
}

TEST(world_entity_count_tracks_create_destroy) {
    World w;
    ASSERT_EQ(w.entity_count(), 0u);
    Entity a = w.create();
    ASSERT_EQ(w.entity_count(), 1u);
    Entity b = w.create();
    ASSERT_EQ(w.entity_count(), 2u);
    w.destroy(a);
    ASSERT_EQ(w.entity_count(), 1u);
    w.destroy(b);
    ASSERT_EQ(w.entity_count(), 0u);
}

TEST(world_index_reused_with_bumped_generation) {
    World w;
    Entity e1 = w.create();
    u32 idx = e1.index();
    u32 gen1 = e1.generation();
    w.destroy(e1);
    Entity e2 = w.create();
    ASSERT_EQ(e2.index(), idx);
    ASSERT_EQ(e2.generation(), gen1 + 1);
}

TEST(world_double_destroy_is_safe) {
    World w;
    Entity e = w.create();
    w.destroy(e);
    // second destroy should be safe (no crash, no assertion in release)
    w.destroy(e);
    ASSERT_FALSE(w.is_alive(e));
}

// =====================================================================
// Component tests
// =====================================================================

TEST(component_add_and_get_roundtrip) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{3.0f, 4.0f});
    Position& pos = w.get_component<Position>(e);
    ASSERT_NEAR(pos.x, 3.0f, 0.001f);
    ASSERT_NEAR(pos.y, 4.0f, 0.001f);
}

TEST(component_has_returns_true_after_add) {
    World w;
    Entity e = w.create();
    ASSERT_FALSE(w.has_component<Position>(e));
    w.add_component<Position>(e, Position{1.0f, 2.0f});
    ASSERT_TRUE(w.has_component<Position>(e));
}

TEST(component_modify_through_reference) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{0, 0});
    Position& pos = w.get_component<Position>(e);
    pos.x = 99.0f;
    pos.y = -1.0f;
    Position& pos2 = w.get_component<Position>(e);
    ASSERT_NEAR(pos2.x, 99.0f, 0.001f);
    ASSERT_NEAR(pos2.y, -1.0f, 0.001f);
}

TEST(component_multiple_different_on_same_entity) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});
    w.add_component<Velocity>(e, Velocity{3, 4});
    w.add_component<Health>(e, Health{50});

    ASSERT_TRUE(w.has_component<Position>(e));
    ASSERT_TRUE(w.has_component<Velocity>(e));
    ASSERT_TRUE(w.has_component<Health>(e));

    ASSERT_NEAR(w.get_component<Position>(e).x, 1.0f, 0.001f);
    ASSERT_NEAR(w.get_component<Velocity>(e).vx, 3.0f, 0.001f);
    ASSERT_EQ(w.get_component<Health>(e).hp, 50);
}

TEST(component_remove_removes_only_that_component) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});
    w.add_component<Velocity>(e, Velocity{3, 4});

    w.remove_component<Velocity>(e);

    ASSERT_TRUE(w.has_component<Position>(e));
    ASSERT_FALSE(w.has_component<Velocity>(e));
    ASSERT_NEAR(w.get_component<Position>(e).x, 1.0f, 0.001f);
}

TEST(component_remove_last_leaves_entity_alive) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});
    w.remove_component<Position>(e);

    ASSERT_TRUE(w.is_alive(e));
    ASSERT_FALSE(w.has_component<Position>(e));
}

// =====================================================================
// Query tests
// =====================================================================

TEST(query_single_component_matches) {
    World w;
    Entity e1 = w.create();
    Entity e2 = w.create();
    w.add_component<Position>(e1, Position{1, 2});
    w.add_component<Velocity>(e2, Velocity{3, 4});

    int count = 0;
    w.query<Position>([&](Entity, Position&) { count++; });
    ASSERT_EQ(count, 1);
}

TEST(query_two_components_matches_both) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});
    w.add_component<Velocity>(e, Velocity{3, 4});

    int count = 0;
    w.query<Position, Velocity>([&](Entity, Position& p, Velocity& v) {
        ASSERT_NEAR(p.x, 1.0f, 0.001f);
        ASSERT_NEAR(v.vx, 3.0f, 0.001f);
        count++;
    });
    ASSERT_EQ(count, 1);
}

TEST(query_two_components_does_not_match_entity_with_only_one) {
    World w;
    Entity e1 = w.create();
    w.add_component<Position>(e1, Position{1, 2});
    // e1 only has Position, not Velocity

    Entity e2 = w.create();
    w.add_component<Position>(e2, Position{5, 6});
    w.add_component<Velocity>(e2, Velocity{7, 8});

    int count = 0;
    w.query<Position, Velocity>([&](Entity, Position&, Velocity&) { count++; });
    ASSERT_EQ(count, 1);
}

TEST(query_iterates_correct_number) {
    World w;
    for (int i = 0; i < 5; i++) {
        Entity e = w.create();
        w.add_component<Position>(e, Position{(float)i, 0});
    }
    for (int i = 0; i < 3; i++) {
        Entity e = w.create();
        w.add_component<Velocity>(e, Velocity{(float)i, 0});
    }

    int pos_count = 0, vel_count = 0;
    w.query<Position>([&](Entity, Position&) { pos_count++; });
    w.query<Velocity>([&](Entity, Velocity&) { vel_count++; });
    ASSERT_EQ(pos_count, 5);
    ASSERT_EQ(vel_count, 3);
}

TEST(query_modify_through_query_and_read_back) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{0, 0});

    w.query<Position>([](Entity, Position& p) {
        p.x = 42.0f;
        p.y = 84.0f;
    });

    Position& pos = w.get_component<Position>(e);
    ASSERT_NEAR(pos.x, 42.0f, 0.001f);
    ASSERT_NEAR(pos.y, 84.0f, 0.001f);
}

TEST(query_with_zero_size_tag_component) {
    World w;
    Entity e1 = w.create();
    w.add_component<Position>(e1, Position{1, 2});
    w.add_component<Tag>(e1);

    Entity e2 = w.create();
    w.add_component<Position>(e2, Position{3, 4});
    // e2 has no Tag

    int count = 0;
    w.query<Position, Tag>([&](Entity, Position& p, Tag&) {
        ASSERT_NEAR(p.x, 1.0f, 0.001f);
        count++;
    });
    ASSERT_EQ(count, 1);
}

TEST(query_after_destruction_excludes_destroyed) {
    World w;
    Entity e1 = w.create();
    Entity e2 = w.create();
    w.add_component<Position>(e1, Position{1, 0});
    w.add_component<Position>(e2, Position{2, 0});

    w.destroy(e1);

    int count = 0;
    w.query<Position>([&](Entity, Position& p) {
        ASSERT_NEAR(p.x, 2.0f, 0.001f);
        count++;
    });
    ASSERT_EQ(count, 1);
}

// =====================================================================
// Archetype movement tests
// =====================================================================

TEST(archetype_adding_component_moves_entity) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});

    ASSERT_TRUE(w.has_component<Position>(e));
    ASSERT_FALSE(w.has_component<Velocity>(e));

    w.add_component<Velocity>(e, Velocity{3, 4});

    ASSERT_TRUE(w.has_component<Position>(e));
    ASSERT_TRUE(w.has_component<Velocity>(e));
    // original data preserved after archetype move
    ASSERT_NEAR(w.get_component<Position>(e).x, 1.0f, 0.001f);
    ASSERT_NEAR(w.get_component<Velocity>(e).vx, 3.0f, 0.001f);
}

TEST(archetype_removing_component_moves_entity) {
    World w;
    Entity e = w.create();
    w.add_component<Position>(e, Position{1, 2});
    w.add_component<Velocity>(e, Velocity{3, 4});

    w.remove_component<Velocity>(e);

    ASSERT_TRUE(w.has_component<Position>(e));
    ASSERT_FALSE(w.has_component<Velocity>(e));
    ASSERT_NEAR(w.get_component<Position>(e).x, 1.0f, 0.001f);
}

TEST(archetype_other_entities_unaffected_by_move) {
    World w;
    Entity e1 = w.create();
    Entity e2 = w.create();
    w.add_component<Position>(e1, Position{1, 2});
    w.add_component<Position>(e2, Position{5, 6});

    // move e1 to a new archetype by adding Velocity
    w.add_component<Velocity>(e1, Velocity{3, 4});

    // e2 should still be in the Position-only archetype, data intact
    ASSERT_TRUE(w.has_component<Position>(e2));
    ASSERT_FALSE(w.has_component<Velocity>(e2));
    ASSERT_NEAR(w.get_component<Position>(e2).x, 5.0f, 0.001f);
    ASSERT_NEAR(w.get_component<Position>(e2).y, 6.0f, 0.001f);
}

TEST(archetype_many_entities_share_archetype) {
    World w;
    std::vector<Entity> entities;
    for (int i = 0; i < 10; i++) {
        Entity e = w.create();
        w.add_component<Position>(e, Position{(float)i, (float)(i * 2)});
        entities.push_back(e);
    }

    // query should find all 10 in one archetype
    int count = 0;
    w.query<Position>([&](Entity, Position&) { count++; });
    ASSERT_EQ(count, 10);

    // verify each entity's data is correct
    for (int i = 0; i < 10; i++) {
        Position& p = w.get_component<Position>(entities[i]);
        ASSERT_NEAR(p.x, (float)i, 0.001f);
        ASSERT_NEAR(p.y, (float)(i * 2), 0.001f);
    }
}

// =====================================================================
// Stress tests
// =====================================================================

TEST(stress_create_1000_query_all) {
    World w;
    for (int i = 0; i < 1000; i++) {
        Entity e = w.create();
        w.add_component<Position>(e, Position{(float)i, 0});
        w.add_component<Velocity>(e, Velocity{0, (float)i});
    }
    ASSERT_EQ(w.entity_count(), 1000u);

    int count = 0;
    w.query<Position, Velocity>([&](Entity, Position&, Velocity&) { count++; });
    ASSERT_EQ(count, 1000);
}

TEST(stress_destroy_half_query_again) {
    World w;
    std::vector<Entity> entities;
    for (int i = 0; i < 1000; i++) {
        Entity e = w.create();
        w.add_component<Position>(e, Position{(float)i, 0});
        entities.push_back(e);
    }

    // destroy even-indexed entities
    for (int i = 0; i < 1000; i += 2) {
        w.destroy(entities[i]);
    }
    ASSERT_EQ(w.entity_count(), 500u);

    int count = 0;
    w.query<Position>([&](Entity, Position&) { count++; });
    ASSERT_EQ(count, 500);
}

// =====================================================================
// for_each_entity tests
// =====================================================================

TEST(for_each_entity_iterates_all_living) {
    World w;
    Entity e1 = w.create();
    Entity e2 = w.create();
    Entity e3 = w.create();
    (void)e1; (void)e2; (void)e3;

    int count = 0;
    w.for_each_entity([&](Entity) { count++; });
    ASSERT_EQ(count, 3);
}

TEST(for_each_entity_skips_destroyed) {
    World w;
    Entity e1 = w.create();
    Entity e2 = w.create();
    Entity e3 = w.create();

    w.destroy(e2);

    int count = 0;
    std::vector<Entity> seen;
    w.for_each_entity([&](Entity e) {
        count++;
        seen.push_back(e);
    });
    ASSERT_EQ(count, 2);

    // verify e2 is not in the seen list
    bool found_e2 = false;
    for (auto& e : seen) {
        if (e == e2) found_e2 = true;
    }
    ASSERT_FALSE(found_e2);
}
