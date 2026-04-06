#include "ecs/archetype.h"

namespace kairo {

Archetype::Archetype(const ComponentSignature& sig) : m_signature(sig) {}

void Archetype::add_column(ComponentTypeId type_id, size_t element_size, size_t alignment) {
    size_t col_index = m_columns.size();
    m_columns.emplace_back(element_size, alignment);
    m_column_map[type_id] = col_index;
}

size_t Archetype::add_entity(Entity entity) {
    size_t row = m_entities.size();
    m_entities.push_back(entity);
    return row;
}

Entity Archetype::remove_entity(size_t row) {
    Entity swapped = NULL_ENTITY;

    if (row < m_entities.size() - 1) {
        // the entity that gets swapped into this row
        swapped = m_entities.back();
        m_entities[row] = swapped;
    }

    m_entities.pop_back();

    // swap-remove in all component columns
    for (auto& col : m_columns) {
        col.swap_remove(row);
    }

    return swapped;
}

ComponentColumn* Archetype::get_column(ComponentTypeId type_id) {
    auto it = m_column_map.find(type_id);
    if (it == m_column_map.end()) return nullptr;
    return &m_columns[it->second];
}

} // namespace kairo
