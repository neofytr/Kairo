#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/world.h"

#include <vector>
#include <functional>

namespace kairo {

// reusable entity pool — avoids create/destroy churn for bullets, particles, etc.
// entities are "deactivated" by adding a PooledInactive tag component,
// and "activated" by removing it. the game skips inactive entities in queries.
//
// usage:
//   pool.init(world, 50, [](World& w, Entity e) { /* add components */ });
//   Entity bullet = pool.acquire();
//   pool.release(bullet);
struct PooledInactive {};

class EntityPool {
public:
    using SetupFunc = std::function<void(World&, Entity)>;

    // pre-allocate `count` entities with the given component setup
    void init(World& world, u32 count, SetupFunc setup) {
        m_world = &world;
        m_setup = std::move(setup);

        for (u32 i = 0; i < count; i++) {
            Entity e = world.create();
            m_setup(world, e);
            world.add_component(e, PooledInactive{});
            m_inactive.push_back(e);
        }
    }

    // get an inactive entity from the pool (or create a new one if empty)
    Entity acquire() {
        if (!m_world) return NULL_ENTITY;

        Entity e;
        if (!m_inactive.empty()) {
            e = m_inactive.back();
            m_inactive.pop_back();
            // reactivate
            if (m_world->has_component<PooledInactive>(e)) {
                m_world->remove_component<PooledInactive>(e);
            }
        } else {
            // pool exhausted — grow
            e = m_world->create();
            m_setup(*m_world, e);
        }

        m_active.push_back(e);
        return e;
    }

    // return an entity to the pool
    void release(Entity e) {
        if (!m_world || !m_world->is_alive(e)) return;

        m_world->add_component(e, PooledInactive{});

        // move from active to inactive list
        for (size_t i = 0; i < m_active.size(); i++) {
            if (m_active[i] == e) {
                m_active[i] = m_active.back();
                m_active.pop_back();
                break;
            }
        }
        m_inactive.push_back(e);
    }

    // release all active entities back to the pool
    void release_all() {
        while (!m_active.empty()) {
            release(m_active.back());
        }
    }

    u32 active_count() const { return static_cast<u32>(m_active.size()); }
    u32 inactive_count() const { return static_cast<u32>(m_inactive.size()); }
    u32 total_count() const { return active_count() + inactive_count(); }

private:
    World* m_world = nullptr;
    SetupFunc m_setup;
    std::vector<Entity> m_active;
    std::vector<Entity> m_inactive;
};

} // namespace kairo
