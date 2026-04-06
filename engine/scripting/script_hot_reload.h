#pragma once

#include "core/types.h"
#include "scripting/script_engine.h"
#include <string>
#include <vector>
#include <filesystem>

namespace kairo {

class ScriptHotReload {
public:
    // watch a script file — will auto-reload via the script engine when modified
    void watch(const std::string& path, ScriptEngine* engine);

    // check all watched files for changes
    void check();

    size_t watched_count() const;

private:
    struct WatchEntry {
        std::string path;
        ScriptEngine* engine;
        std::filesystem::file_time_type last_modified;
    };
    std::vector<WatchEntry> m_entries;
};

} // namespace kairo
