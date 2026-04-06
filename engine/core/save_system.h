#pragma once

#include "core/types.h"
#include <string>
#include <json.hpp>

namespace kairo {

// simple key-value save system that persists to JSON files
class SaveSystem {
public:
    // set values
    void set_int(const std::string& key, int value);
    void set_float(const std::string& key, float value);
    void set_string(const std::string& key, const std::string& value);
    void set_bool(const std::string& key, bool value);

    // get values (with defaults)
    int get_int(const std::string& key, int default_val = 0) const;
    float get_float(const std::string& key, float default_val = 0.0f) const;
    std::string get_string(const std::string& key, const std::string& default_val = "") const;
    bool get_bool(const std::string& key, bool default_val = false) const;

    bool has_key(const std::string& key) const;
    void remove(const std::string& key);
    void clear();

    // save/load to file
    bool save(const std::string& path) const;
    bool load(const std::string& path);

private:
    nlohmann::json m_data;
};

} // namespace kairo
