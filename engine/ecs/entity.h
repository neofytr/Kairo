#pragma once

#include "core/types.h"

namespace kairo {

// entity handle: 32 bits for index, 32 bits for generation
// generation detects use-after-destroy (stale handles)
struct Entity {
    u64 id = 0;

    Entity() = default;
    explicit constexpr Entity(u64 id) : id(id) {}

    u32 index() const { return static_cast<u32>(id & 0xFFFFFFFF); }
    u32 generation() const { return static_cast<u32>(id >> 32); }

    bool is_valid() const { return id != 0; }
    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }

    static Entity make(u32 index, u32 generation) {
        return Entity(static_cast<u64>(generation) << 32 | index);
    }
};

// null entity constant
inline constexpr Entity NULL_ENTITY = Entity(0);

} // namespace kairo

// hash support for Entity so it can be used as a map key
namespace std {
template<>
struct hash<kairo::Entity> {
    size_t operator()(const kairo::Entity& e) const {
        return std::hash<kairo::u64>()(e.id);
    }
};
} // namespace std
