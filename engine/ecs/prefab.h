#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/world.h"

#include <json.hpp>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace kairo {

using json = nlohmann::json;

// a prefab is a template for creating entities with predefined components
class PrefabRegistry {
public:
    // register how to serialize/deserialize a component type
    template<typename T>
    void register_component(const std::string& name,
        std::function<json(const T&)> to_json,
        std::function<T(const json&)> from_json,
        std::function<bool(World&, Entity)> has_check);

    // save an existing entity as a prefab file
    bool save_prefab(World& world, Entity entity, const std::string& path);

    // load a prefab file and return the JSON (doesn't instantiate)
    json load_prefab(const std::string& path);

    // instantiate a prefab from JSON into the world, returns new entity
    Entity instantiate(World& world, const json& prefab_data);

    // instantiate directly from a file
    Entity instantiate_from_file(World& world, const std::string& path);

    // store a named prefab in memory (no file)
    void store(const std::string& name, const json& data);
    Entity instantiate_by_name(World& world, const std::string& name);

    bool has_prefab(const std::string& name) const;

private:
    struct ComponentHandler {
        std::string name;
        // serialize: check if entity has component, write to json
        std::function<void(World&, Entity, json&)> serialize;
        // deserialize: read from json, add component to entity
        std::function<void(World&, Entity, const json&)> deserialize;
    };

    std::vector<ComponentHandler> m_handlers;
    std::unordered_map<std::string, json> m_stored_prefabs;
};

// --- template implementation ---

template<typename T>
void PrefabRegistry::register_component(const std::string& name,
    std::function<json(const T&)> to_json,
    std::function<T(const json&)> from_json,
    std::function<bool(World&, Entity)> has_check) {

    ComponentHandler handler;
    handler.name = name;

    handler.serialize = [name, to_json, has_check](World& world, Entity entity, json& out) {
        if (has_check(world, entity)) {
            out["components"][name] = to_json(world.get_component<T>(entity));
        }
    };

    handler.deserialize = [from_json](World& world, Entity entity, const json& comp_json) {
        world.add_component<T>(entity, from_json(comp_json));
    };

    m_handlers.push_back(std::move(handler));
}

} // namespace kairo
