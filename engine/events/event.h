#pragma once

#include "core/types.h"
#include <typeindex>

namespace kairo {

// base event type — all events inherit from this
// events are value types, no virtual dispatch
struct Event {
    // type ID used for dispatch, set automatically by EventBus
};

// get a unique ID for an event type
template<typename T>
std::type_index get_event_id() {
    return std::type_index(typeid(T));
}

// --- built-in events ---

struct CollisionEvent {
    u64 entity_a;
    u64 entity_b;
    float penetration;
};

struct EntityDestroyedEvent {
    u64 entity_id;
};

struct SceneChangedEvent {
    const char* scene_name;
};

struct WindowResizedEvent {
    i32 width;
    i32 height;
};

} // namespace kairo
