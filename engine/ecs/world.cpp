#include "ecs/world.h"
#include "core/log.h"

namespace kairo {

Entity World::create() {
    u32 index;
    u32 generation;

    if (!m_free_indices.empty()) {
        // reuse a recycled slot
        index = m_free_indices.back();
        m_free_indices.pop_back();
        generation = m_records[index].generation;
    } else {
        // allocate a new slot
        index = static_cast<u32>(m_records.size());
        generation = 1; // generation 0 is reserved (NULL_ENTITY)
        m_records.push_back({});
    }

    auto& record = m_records[index];
    record.generation = generation;
    record.alive = true;
    record.archetype = nullptr;
    record.row = 0;

    return Entity::make(index, generation);
}

void World::destroy(Entity entity) {
    if (!is_alive(entity)) return;

    auto& record = m_records[entity.index()];

    // remove from archetype if it has one
    if (record.archetype) {
        Entity swapped = record.archetype->remove_entity(record.row);
        if (swapped.is_valid()) {
            m_records[swapped.index()].row = record.row;
        }
    }

    record.alive = false;
    record.archetype = nullptr;
    record.generation++; // bump generation so old handles become invalid
    m_free_indices.push_back(entity.index());
}

bool World::is_alive(Entity entity) const {
    u32 idx = entity.index();
    if (idx >= m_records.size()) return false;
    return m_records[idx].alive && m_records[idx].generation == entity.generation();
}

size_t World::entity_count() const {
    size_t count = 0;
    for (auto& r : m_records) {
        if (r.alive) count++;
    }
    return count;
}

Archetype* World::get_or_create_archetype(const ComponentSignature& sig) {
    auto it = m_archetypes.find(sig);
    if (it != m_archetypes.end()) {
        return it->second.get();
    }

    // create new archetype with columns for each component type
    auto arch = std::make_unique<Archetype>(sig);
    for (auto& type_id : sig.types) {
        auto& meta = m_component_meta[type_id];
        arch->add_column(type_id, meta.size, meta.alignment, meta.destroy);
    }

    Archetype* ptr = arch.get();
    m_archetypes[sig] = std::move(arch);
    return ptr;
}

void World::move_entity(Entity entity, Archetype* from, size_t from_row,
                        Archetype* to, const ComponentSignature& new_sig) {
    // add entity to new archetype
    size_t new_row = to->add_entity(entity);

    // allocate space in all columns of the new archetype
    for (auto& type_id : new_sig.types) {
        auto* col = to->get_column(type_id);
        col->push_back();
    }

    // copy component data that exists in both old and new archetypes
    const auto& old_sig = from->get_signature();
    for (auto& type_id : old_sig.types) {
        if (new_sig.contains(type_id)) {
            auto* src_col = from->get_column(type_id);
            auto* dst_col = to->get_column(type_id);
            std::memcpy(dst_col->get_raw(new_row), src_col->get_raw(from_row),
                        src_col->element_size());
        }
    }

    // remove from old archetype (swap-remove)
    Entity swapped = from->remove_entity(from_row);
    if (swapped.is_valid()) {
        m_records[swapped.index()].row = from_row;
    }

    // update record
    auto& record = m_records[entity.index()];
    record.archetype = to;
    record.row = new_row;
}

} // namespace kairo
