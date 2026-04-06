#include "scene/scene_serializer.h"
#include "core/log.h"

#include <fstream>
#include <sstream>

namespace kairo {

std::string SceneSerializer::serialize(World& world) const {
    json root;
    root["version"] = 1;
    root["entities"] = json::array();

    world.for_each_entity([&](Entity entity) {
        json entity_json;
        entity_json["id"] = entity.id;
        entity_json["components"] = json::object();

        // try each registered serializer
        for (auto& [name, serializer] : m_serializers) {
            serializer.serialize(world, entity, entity_json);
        }

        // only include entities that have at least one serializable component
        if (!entity_json["components"].empty()) {
            root["entities"].push_back(entity_json);
        }
    });

    root["entity_count"] = root["entities"].size();

    return root.dump(2);
}

bool SceneSerializer::save(World& world, const std::string& path) const {
    std::string data = serialize(world);

    std::ofstream file(path);
    if (!file.is_open()) {
        log::error("scene serializer: failed to open '%s' for writing", path.c_str());
        return false;
    }

    file << data;
    log::info("scene saved to '%s' (%zu bytes)", path.c_str(), data.size());
    return true;
}

bool SceneSerializer::deserialize(World& world, const std::string& json_str) const {
    json root;
    try {
        root = json::parse(json_str);
    } catch (const json::exception& e) {
        log::error("scene serializer: JSON parse error: %s", e.what());
        return false;
    }

    if (!root.contains("entities")) {
        log::warn("scene serializer: no entities array found");
        return true;
    }

    for (auto& entity_json : root["entities"]) {
        Entity entity = world.create();

        if (entity_json.contains("components")) {
            for (auto& [name, comp_data] : entity_json["components"].items()) {
                auto it = m_serializers.find(name);
                if (it != m_serializers.end()) {
                    it->second.deserialize(world, entity, comp_data);
                } else {
                    log::warn("scene serializer: unknown component '%s', skipping", name.c_str());
                }
            }
        }
    }

    log::info("scene loaded: %zu entities", world.entity_count());
    return true;
}

bool SceneSerializer::load(World& world, const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        log::error("scene serializer: failed to open '%s'", path.c_str());
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    return deserialize(world, ss.str());
}

} // namespace kairo
