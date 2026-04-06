#include "test_framework.h"
#include "events/event_bus.h"

using namespace kairo;

// ---- test events (local to this file) ----
struct DamageEvent { int amount; };
struct HealEvent { int amount; };
struct EmptyEvent {};

// =====================================================================
// Subscribe and publish tests
// =====================================================================

TEST(event_subscribe_and_publish_fires_callback) {
    EventBus bus;
    bool fired = false;
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { fired = true; });
    bus.publish(DamageEvent{10});
    ASSERT_TRUE(fired);
}

TEST(event_callback_receives_correct_data) {
    EventBus bus;
    int received_amount = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) {
        received_amount = e.amount;
    });
    bus.publish(DamageEvent{42});
    ASSERT_EQ(received_amount, 42);
}

TEST(event_multiple_subscribers_all_receive) {
    EventBus bus;
    int count = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.publish(DamageEvent{5});
    ASSERT_EQ(count, 3);
}

TEST(event_unsubscribe_stops_delivery) {
    EventBus bus;
    int count = 0;
    u32 id = bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.publish(DamageEvent{1});
    ASSERT_EQ(count, 1);

    bus.unsubscribe(id);
    bus.publish(DamageEvent{2});
    ASSERT_EQ(count, 1);  // should not increment
}

TEST(event_publish_unsubscribed_type_no_crash) {
    EventBus bus;
    // no subscribers for DamageEvent; this should not crash
    bus.publish(DamageEvent{100});
    ASSERT_TRUE(true);  // if we get here, no crash
}

// =====================================================================
// Queue and flush tests
// =====================================================================

TEST(event_queue_and_flush_delivers_in_order) {
    EventBus bus;
    std::vector<int> received;
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) {
        received.push_back(e.amount);
    });

    bus.queue(DamageEvent{1});
    bus.queue(DamageEvent{2});
    bus.queue(DamageEvent{3});

    // nothing should be delivered yet
    ASSERT_EQ(received.size(), 0u);

    bus.flush();
    ASSERT_EQ(received.size(), 3u);
    ASSERT_EQ(received[0], 1);
    ASSERT_EQ(received[1], 2);
    ASSERT_EQ(received[2], 3);
}

TEST(event_flush_clears_the_queue) {
    EventBus bus;
    int count = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });

    bus.queue(DamageEvent{1});
    bus.flush();
    ASSERT_EQ(count, 1);

    // second flush should not deliver anything more
    bus.flush();
    ASSERT_EQ(count, 1);
}

TEST(event_clear_removes_all_handlers) {
    EventBus bus;
    int count = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.subscribe<HealEvent>([&](const HealEvent&) { count++; });

    bus.clear();

    bus.publish(DamageEvent{1});
    bus.publish(HealEvent{1});
    ASSERT_EQ(count, 0);
}

// =====================================================================
// Type isolation tests
// =====================================================================

TEST(event_different_types_dont_crosstalk) {
    EventBus bus;
    int damage_count = 0;
    int heal_count = 0;

    bus.subscribe<DamageEvent>([&](const DamageEvent&) { damage_count++; });
    bus.subscribe<HealEvent>([&](const HealEvent&) { heal_count++; });

    bus.publish(DamageEvent{10});
    ASSERT_EQ(damage_count, 1);
    ASSERT_EQ(heal_count, 0);

    bus.publish(HealEvent{20});
    ASSERT_EQ(damage_count, 1);
    ASSERT_EQ(heal_count, 1);
}

TEST(event_different_types_correct_data) {
    EventBus bus;
    int damage_val = 0;
    int heal_val = 0;

    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { damage_val = e.amount; });
    bus.subscribe<HealEvent>([&](const HealEvent& e) { heal_val = e.amount; });

    bus.publish(DamageEvent{25});
    bus.publish(HealEvent{50});

    ASSERT_EQ(damage_val, 25);
    ASSERT_EQ(heal_val, 50);
}

// =====================================================================
// Re-entrancy test
// =====================================================================

TEST(event_publish_during_callback_works) {
    EventBus bus;
    int damage_count = 0;
    int heal_count = 0;

    bus.subscribe<HealEvent>([&](const HealEvent&) { heal_count++; });

    bus.subscribe<DamageEvent>([&](const DamageEvent& e) {
        damage_count++;
        // re-entrant publish: taking damage triggers a heal
        bus.publish(HealEvent{e.amount / 2});
    });

    bus.publish(DamageEvent{100});
    ASSERT_EQ(damage_count, 1);
    ASSERT_EQ(heal_count, 1);
}

// =====================================================================
// Edge-case and additional tests
// =====================================================================

TEST(event_empty_event_works) {
    EventBus bus;
    bool fired = false;
    bus.subscribe<EmptyEvent>([&](const EmptyEvent&) { fired = true; });
    bus.publish(EmptyEvent{});
    ASSERT_TRUE(fired);
}

TEST(event_unsubscribe_one_leaves_others) {
    EventBus bus;
    int count_a = 0, count_b = 0;
    u32 id_a = bus.subscribe<DamageEvent>([&](const DamageEvent&) { count_a++; });
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count_b++; });

    bus.unsubscribe(id_a);
    bus.publish(DamageEvent{1});

    ASSERT_EQ(count_a, 0);
    ASSERT_EQ(count_b, 1);
}

TEST(event_unsubscribe_invalid_id_is_safe) {
    EventBus bus;
    // unsubscribing a non-existent ID should not crash
    bus.unsubscribe(9999);
    ASSERT_TRUE(true);
}

TEST(event_multiple_publishes_accumulate) {
    EventBus bus;
    int total = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { total += e.amount; });

    bus.publish(DamageEvent{10});
    bus.publish(DamageEvent{20});
    bus.publish(DamageEvent{30});

    ASSERT_EQ(total, 60);
}

TEST(event_queue_mixed_types) {
    EventBus bus;
    std::vector<int> order;

    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { order.push_back(e.amount); });
    bus.subscribe<HealEvent>([&](const HealEvent& e) { order.push_back(-e.amount); });

    bus.queue(DamageEvent{1});
    bus.queue(HealEvent{2});
    bus.queue(DamageEvent{3});

    bus.flush();

    ASSERT_EQ(order.size(), 3u);
    ASSERT_EQ(order[0], 1);
    ASSERT_EQ(order[1], -2);
    ASSERT_EQ(order[2], 3);
}

TEST(event_clear_also_clears_queued) {
    EventBus bus;
    int count = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.queue(DamageEvent{1});
    bus.clear();
    bus.flush();
    ASSERT_EQ(count, 0);
}

TEST(event_subscribe_after_publish) {
    EventBus bus;
    int count = 0;
    bus.publish(DamageEvent{10});
    bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.publish(DamageEvent{20});
    ASSERT_EQ(count, 1);  // only the second publish should be received
}

TEST(event_multiple_unsubscribe_same_id) {
    EventBus bus;
    int count = 0;
    u32 id = bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    bus.unsubscribe(id);
    bus.unsubscribe(id);  // second unsubscribe should be safe
    bus.publish(DamageEvent{1});
    ASSERT_EQ(count, 0);
}

TEST(event_stress_many_subscribers) {
    EventBus bus;
    int count = 0;
    for (int i = 0; i < 100; i++) {
        bus.subscribe<DamageEvent>([&](const DamageEvent&) { count++; });
    }
    bus.publish(DamageEvent{1});
    ASSERT_EQ(count, 100);
}

TEST(event_queue_during_flush) {
    EventBus bus;
    std::vector<int> received;

    bus.subscribe<DamageEvent>([&](const DamageEvent& e) {
        received.push_back(e.amount);
        if (e.amount == 1) {
            // queue another event during flush processing
            bus.queue(DamageEvent{2});
        }
    });

    bus.queue(DamageEvent{1});
    bus.flush();

    // The flush loop processes newly queued events too (index-based loop)
    ASSERT_EQ(received.size(), 2u);
    ASSERT_EQ(received[0], 1);
    ASSERT_EQ(received[1], 2);
}
