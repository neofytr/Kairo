#include "core/save_system.h"
#include "core/log.h"
#include <fstream>

namespace kairo {

// --- setters ---

void SaveSystem::set_int(const std::string& key, int value) {
    m_data[key] = value;
}

void SaveSystem::set_float(const std::string& key, float value) {
    m_data[key] = value;
}

void SaveSystem::set_string(const std::string& key, const std::string& value) {
    m_data[key] = value;
}

void SaveSystem::set_bool(const std::string& key, bool value) {
    m_data[key] = value;
}

// --- getters (return default if key is missing) ---

int SaveSystem::get_int(const std::string& key, int default_val) const {
    return m_data.value(key, default_val);
}

float SaveSystem::get_float(const std::string& key, float default_val) const {
    return m_data.value(key, default_val);
}

std::string SaveSystem::get_string(const std::string& key, const std::string& default_val) const {
    return m_data.value(key, default_val);
}

bool SaveSystem::get_bool(const std::string& key, bool default_val) const {
    return m_data.value(key, default_val);
}

// --- utility ---

bool SaveSystem::has_key(const std::string& key) const {
    return m_data.contains(key);
}

void SaveSystem::remove(const std::string& key) {
    m_data.erase(key);
}

void SaveSystem::clear() {
    m_data.clear();
}

// --- persistence ---

bool SaveSystem::save(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        log::error("SaveSystem: failed to open '%s' for writing", path.c_str());
        return false;
    }
    file << m_data.dump(2);
    return true;
}

bool SaveSystem::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        log::error("SaveSystem: failed to open '%s' for reading", path.c_str());
        return false;
    }
    try {
        file >> m_data;
    } catch (const nlohmann::json::parse_error& e) {
        log::error("SaveSystem: parse error in '%s': %s", path.c_str(), e.what());
        return false;
    }
    return true;
}

} // namespace kairo
