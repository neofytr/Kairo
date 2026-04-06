#pragma once

#include "core/types.h"
#include "ecs/world.h"
#include "ecs/entity.h"
#include "ecs/component.h"

#include <json.hpp>
#include <string>
#include <functional>
#include <unordered_map>

namespace kairo {

using json = nlohmann::json;

// serialization/deserialization functions for a component type
struct ComponentSerializer {
    // serialize: read component from entity, write to json
    std::function<void(World&, Entity, json&)> serialize;
    // deserialize: read from json, add component to entity
    std::function<void(World&, Entity, const json&)> deserialize;
};

// handles saving and loading ECS worlds to/from JSON
class SceneSerializer {
public:
    // register a component type with its name and serialize/deserialize functions
    template<typename T>
    void register_component(const std::string& name,
        std::function<json(const T&)> to_json,
        std::function<T(const json&)> from_json);

    // serialize the world to a JSON string
    std::string serialize(World& world) const;

    // save to file
    bool save(World& world, const std::string& path) const;

    // deserialize from JSON string, populating the world
    bool deserialize(World& world, const std::string& json_str) const;

    // load from file
    bool load(World& world, const std::string& path) const;

private:
    std::unordered_map<std::string, ComponentSerializer> m_serializers;

    // maps component type ID → registered name (for serialization)
    std::unordered_map<ComponentTypeId, std::string> m_type_to_name;
};

template<typename T>
void SceneSerializer::register_component(const std::string& name,
    std::function<json(const T&)> to_json,
    std::function<T(const json&)> from_json) {

    auto type_id = get_component_id<T>();
    m_type_to_name[type_id] = name;

    ComponentSerializer cs;
    cs.serialize = [name, type_id, to_json](World& world, Entity entity, json& entity_json) {
        if (world.has_component<T>(entity)) {
            entity_json["components"][name] = to_json(world.get_component<T>(entity));
        }
    };
    cs.deserialize = [from_json](World& world, Entity entity, const json& comp_json) {
        world.add_component<T>(entity, from_json(comp_json));
    };

    m_serializers[name] = cs;
}

} // namespace kairo
