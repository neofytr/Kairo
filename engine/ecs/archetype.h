#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/component.h"

#include <vector>
#include <unordered_map>
#include <cstring>
#include <cassert>

namespace kairo {

// type-erased column of component data
// stores a contiguous array of one component type
class ComponentColumn {
public:
    ComponentColumn() = default;
    ComponentColumn(size_t element_size, size_t alignment)
        : m_element_size(element_size), m_alignment(alignment) {}

    // add a new element, returns pointer to the new slot
    void* push_back() {
        m_data.resize(m_data.size() + m_element_size);
        m_count++;
        return &m_data[m_data.size() - m_element_size];
    }

    // copy raw bytes into a new slot
    void push_back_copy(const void* src) {
        void* dst = push_back();
        std::memcpy(dst, src, m_element_size);
    }

    // remove element at index by swapping with the last element
    void swap_remove(size_t index) {
        assert(index < m_count);
        if (index < m_count - 1) {
            // swap with last
            void* dst = get_raw(index);
            void* src = get_raw(m_count - 1);
            std::memcpy(dst, src, m_element_size);
        }
        m_data.resize(m_data.size() - m_element_size);
        m_count--;
    }

    void* get_raw(size_t index) {
        return &m_data[index * m_element_size];
    }

    const void* get_raw(size_t index) const {
        return &m_data[index * m_element_size];
    }

    template<typename T>
    T& get(size_t index) {
        return *reinterpret_cast<T*>(get_raw(index));
    }

    template<typename T>
    const T& get(size_t index) const {
        return *reinterpret_cast<const T*>(get_raw(index));
    }

    size_t size() const { return m_count; }
    size_t element_size() const { return m_element_size; }

private:
    std::vector<u8> m_data;
    size_t m_element_size = 0;
    size_t m_alignment = 1;
    size_t m_count = 0;
};

// an archetype stores all entities that have the exact same set of components
// components are stored in parallel arrays (one column per component type)
class Archetype {
public:
    Archetype() = default;
    explicit Archetype(const ComponentSignature& sig);

    const ComponentSignature& get_signature() const { return m_signature; }

    // register a component type's column (called during archetype setup)
    void add_column(ComponentTypeId type_id, size_t element_size, size_t alignment);

    // add an entity to this archetype, returns its row index
    size_t add_entity(Entity entity);

    // remove an entity by row index (swap-remove)
    // returns the entity that was swapped into this slot (or NULL_ENTITY if it was the last)
    Entity remove_entity(size_t row);

    // get component data for a specific type at a given row
    template<typename T>
    T& get_component(size_t row) {
        auto it = m_column_map.find(get_component_id<T>());
        assert(it != m_column_map.end());
        return m_columns[it->second].template get<T>(row);
    }

    // get raw column by type id
    ComponentColumn* get_column(ComponentTypeId type_id);

    // how many entities in this archetype
    size_t entity_count() const { return m_entities.size(); }

    // get entity at row
    Entity get_entity(size_t row) const { return m_entities[row]; }

    // check if this archetype has a given component type
    bool has_component(ComponentTypeId type_id) const {
        return m_column_map.find(type_id) != m_column_map.end();
    }

private:
    ComponentSignature m_signature;
    std::vector<Entity> m_entities;
    std::vector<ComponentColumn> m_columns;
    std::unordered_map<ComponentTypeId, size_t> m_column_map; // type -> column index
};

} // namespace kairo
