#include "ecs/prefab.h"
#include "core/log.h"

#include <fstream>
#include <sstream>

namespace kairo {

bool PrefabRegistry::save_prefab(World& world, Entity entity, const std::string& path) {
    json prefab;
    prefab["version"] = 1;
    prefab["components"] = json::object();

    // iterate all registered handlers and serialize matching components
    for (auto& handler : m_handlers) {
        handler.serialize(world, entity, prefab);
    }

    if (prefab["components"].empty()) {
        log::warn("prefab: entity has no serializable components");
        return false;
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        log::error("prefab: failed to open '%s' for writing", path.c_str());
        return false;
    }

    file << prefab.dump(2);
    log::info("prefab saved to '%s'", path.c_str());
    return true;
}

json PrefabRegistry::load_prefab(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        log::error("prefab: failed to open '%s'", path.c_str());
        return json{};
    }

    std::stringstream ss;
    ss << file.rdbuf();

    try {
        return json::parse(ss.str());
    } catch (const json::exception& e) {
        log::error("prefab: JSON parse error in '%s': %s", path.c_str(), e.what());
        return json{};
    }
}

Entity PrefabRegistry::instantiate(World& world, const json& prefab_data) {
    if (!prefab_data.contains("components")) {
        log::warn("prefab: data has no components");
        return NULL_ENTITY;
    }

    Entity entity = world.create();

    for (auto& [name, comp_data] : prefab_data["components"].items()) {
        // find the handler matching this component name
        bool found = false;
        for (auto& handler : m_handlers) {
            if (handler.name == name) {
                handler.deserialize(world, entity, comp_data);
                found = true;
                break;
            }
        }
        if (!found) {
            log::warn("prefab: unknown component '%s', skipping", name.c_str());
        }
    }

    return entity;
}

Entity PrefabRegistry::instantiate_from_file(World& world, const std::string& path) {
    json data = load_prefab(path);
    if (data.is_null() || data.empty()) {
        return NULL_ENTITY;
    }
    return instantiate(world, data);
}

void PrefabRegistry::store(const std::string& name, const json& data) {
    m_stored_prefabs[name] = data;
}

Entity PrefabRegistry::instantiate_by_name(World& world, const std::string& name) {
    auto it = m_stored_prefabs.find(name);
    if (it == m_stored_prefabs.end()) {
        log::error("prefab: no stored prefab named '%s'", name.c_str());
        return NULL_ENTITY;
    }
    return instantiate(world, it->second);
}

bool PrefabRegistry::has_prefab(const std::string& name) const {
    return m_stored_prefabs.find(name) != m_stored_prefabs.end();
}

} // namespace kairo
