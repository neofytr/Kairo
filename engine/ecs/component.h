#pragma once

#include "core/types.h"
#include <typeindex>
#include <typeinfo>
#include <vector>
#include <algorithm>

namespace kairo {

// each component type gets a unique ID based on std::type_index
// this is used as the key for archetype lookups
using ComponentTypeId = std::type_index;

template<typename T>
ComponentTypeId get_component_id() {
    return std::type_index(typeid(T));
}

// a sorted set of component type IDs that defines an archetype
// two entities with the same ComponentSignature live in the same archetype
struct ComponentSignature {
    std::vector<ComponentTypeId> types;

    void add(ComponentTypeId id) {
        auto it = std::lower_bound(types.begin(), types.end(), id);
        if (it == types.end() || *it != id) {
            types.insert(it, id);
        }
    }

    void remove(ComponentTypeId id) {
        auto it = std::lower_bound(types.begin(), types.end(), id);
        if (it != types.end() && *it == id) {
            types.erase(it);
        }
    }

    bool contains(ComponentTypeId id) const {
        return std::binary_search(types.begin(), types.end(), id);
    }

    bool operator==(const ComponentSignature& other) const {
        return types == other.types;
    }
};

} // namespace kairo

// hash for ComponentSignature — used to look up archetypes quickly
namespace std {
template<>
struct hash<kairo::ComponentSignature> {
    size_t operator()(const kairo::ComponentSignature& sig) const {
        size_t seed = sig.types.size();
        for (auto& t : sig.types) {
            // combine hashes using a standard technique
            seed ^= t.hash_code() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
} // namespace std
