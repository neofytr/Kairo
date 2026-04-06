#pragma once
#include "core/types.h"

#include <algorithm>

namespace kairo {

struct HealthComponent {
    float current = 100.0f;
    float max = 100.0f;

    bool is_alive() const { return current > 0.0f; }
    float ratio() const { return max > 0 ? current / max : 0; }

    // clamp to max
    void heal(float amount) {
        current = std::min(current + amount, max);
    }

    // clamp to 0, invincibility check happens at system level
    void damage(float amount) {
        current = std::max(current - amount, 0.0f);
    }

    // set new max; heals to full if max increased
    void set_max(float m) {
        bool increasing = m > max;
        max = m;
        if (increasing) current = max;
        current = std::min(current, max);
    }
};

struct DamageEvent {
    u64 target_entity;
    u64 source_entity;
    float amount;
};

struct DeathEvent {
    u64 entity_id;
    u64 killer_id;
};

// invincibility frames — entity can't take damage while active
struct InvincibilityComponent {
    float duration = 0.0f;   // total i-frame duration
    float remaining = 0.0f;  // time left

    bool is_active() const { return remaining > 0.0f; }
    void activate() { remaining = duration; }
    void activate(float dur) { duration = dur; remaining = dur; }
    void update(float dt) { if (remaining > 0) remaining -= dt; }
};

} // namespace kairo
