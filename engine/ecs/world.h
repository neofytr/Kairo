#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/archetype.h"

#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <tuple>

namespace kairo {

class World {
public:
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    Entity create();
    void destroy(Entity entity);
    bool is_alive(Entity entity) const;

    template<typename T>
    T& add_component(Entity entity, const T& value = T{});

    template<typename T>
    void remove_component(Entity entity);

    template<typename T>
    T& get_component(Entity entity);

    template<typename T>
    bool has_component(Entity entity) const;

    // query all entities with the given component types
    // usage: world.query<Transform, Sprite>([](Entity e, Transform& t, Sprite& s) { ... });
    template<typename... Ts, typename Func>
    void query(Func&& func);

    size_t entity_count() const;

    // iterate over all living entities
    template<typename Func>
    void for_each_entity(Func&& func) const {
        for (u32 i = 0; i < m_records.size(); i++) {
            if (m_records[i].alive) {
                func(Entity::make(i, m_records[i].generation));
            }
        }
    }

private:
    struct EntityRecord {
        Archetype* archetype = nullptr;
        size_t row = 0;
        u32 generation = 0;
        bool alive = false;
    };

    std::vector<EntityRecord> m_records;
    std::vector<u32> m_free_indices;
    std::unordered_map<ComponentSignature, std::unique_ptr<Archetype>> m_archetypes;

    struct ComponentMeta {
        size_t size;
        size_t alignment;
        DestroyFunc destroy;
    };
    std::unordered_map<ComponentTypeId, ComponentMeta> m_component_meta;

    template<typename T>
    void ensure_component_registered();

    Archetype* get_or_create_archetype(const ComponentSignature& sig);

    void move_entity(Entity entity, Archetype* from, size_t from_row,
                     Archetype* to, const ComponentSignature& new_sig);
};

// --- template implementations ---

template<typename T>
void World::ensure_component_registered() {
    auto id = get_component_id<T>();
    if (m_component_meta.find(id) == m_component_meta.end()) {
        // only store a destructor for non-trivially-destructible types
        DestroyFunc destroy = nullptr;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            destroy = [](void* ptr) { static_cast<T*>(ptr)->~T(); };
        }
        m_component_meta[id] = { sizeof(T), alignof(T), destroy };
    }
}

template<typename T>
T& World::add_component(Entity entity, const T& value) {
    ensure_component_registered<T>();
    auto id = get_component_id<T>();

    auto& record = m_records[entity.index()];
    assert(record.alive && record.generation == entity.generation());

    ComponentSignature new_sig;
    if (record.archetype) {
        new_sig = record.archetype->get_signature();
    }
    new_sig.add(id);

    Archetype* new_arch = get_or_create_archetype(new_sig);

    if (record.archetype) {
        move_entity(entity, record.archetype, record.row, new_arch, new_sig);
    } else {
        size_t row = new_arch->add_entity(entity);
        for (auto& type_id : new_sig.types) {
            auto* col = new_arch->get_column(type_id);
            col->push_back();
        }
        record.archetype = new_arch;
        record.row = row;
    }

    auto* col = new_arch->get_column(id);
    T& component = col->template get<T>(record.row);
    component = value;
    return component;
}

template<typename T>
void World::remove_component(Entity entity) {
    auto id = get_component_id<T>();
    auto& record = m_records[entity.index()];
    assert(record.alive && record.generation == entity.generation());
    assert(record.archetype);

    ComponentSignature new_sig = record.archetype->get_signature();
    new_sig.remove(id);

    if (new_sig.types.empty()) {
        Entity swapped = record.archetype->remove_entity(record.row);
        if (swapped.is_valid()) {
            m_records[swapped.index()].row = record.row;
        }
        record.archetype = nullptr;
        record.row = 0;
    } else {
        Archetype* new_arch = get_or_create_archetype(new_sig);
        move_entity(entity, record.archetype, record.row, new_arch, new_sig);
    }
}

template<typename T>
T& World::get_component(Entity entity) {
    auto& record = m_records[entity.index()];
    assert(record.alive && record.generation == entity.generation());
    assert(record.archetype);
    return record.archetype->get_component<T>(record.row);
}

template<typename T>
bool World::has_component(Entity entity) const {
    auto& record = m_records[entity.index()];
    if (!record.alive || record.generation != entity.generation() || !record.archetype) {
        return false;
    }
    return record.archetype->has_component(get_component_id<T>());
}

namespace detail {
// helper to call func with the right component references from column pointers
template<typename... Ts, typename Func, size_t... Is>
void call_with_components(Func&& func, Entity entity,
                          ComponentColumn** columns, size_t row,
                          std::index_sequence<Is...>) {
    func(entity, columns[Is]->get<Ts>(row)...);
}
} // namespace detail

template<typename... Ts, typename Func>
void World::query(Func&& func) {
    ComponentTypeId required[] = { get_component_id<Ts>()... };
    constexpr size_t N = sizeof...(Ts);

    for (auto& [sig, archetype] : m_archetypes) {
        // skip archetypes missing any required component
        bool matches = true;
        for (size_t i = 0; i < N; i++) {
            if (!archetype->has_component(required[i])) {
                matches = false;
                break;
            }
        }
        if (!matches) continue;

        // grab column pointers once per archetype
        ComponentColumn* columns[N] = { archetype->get_column(get_component_id<Ts>())... };

        size_t count = archetype->entity_count();
        for (size_t i = 0; i < count; i++) {
            Entity entity = archetype->get_entity(i);
            detail::call_with_components<Ts...>(
                std::forward<Func>(func), entity, columns, i,
                std::index_sequence_for<Ts...>{});
        }
    }
}

} // namespace kairo
