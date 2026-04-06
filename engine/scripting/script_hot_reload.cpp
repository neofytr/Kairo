#include "scripting/script_hot_reload.h"
#include "core/log.h"

namespace kairo {

void ScriptHotReload::watch(const std::string& path, ScriptEngine* engine) {
    WatchEntry entry;
    entry.path = path;
    entry.engine = engine;

    // grab initial timestamp
    try {
        entry.last_modified = std::filesystem::last_write_time(path);
    } catch (const std::filesystem::filesystem_error& e) {
        log::warn("script_hot_reload: failed to read timestamp for %s: %s",
                  path.c_str(), e.what());
        return;
    }

    m_entries.push_back(std::move(entry));
}

void ScriptHotReload::check() {
    for (auto& entry : m_entries) {
        try {
            auto now = std::filesystem::last_write_time(entry.path);
            if (now != entry.last_modified) {
                log::info("script_hot_reload: reloading [%s]", entry.path.c_str());
                entry.engine->load_file(entry.path);
                entry.last_modified = now;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            log::warn("script_hot_reload: filesystem error: %s", e.what());
        }
    }
}

size_t ScriptHotReload::watched_count() const {
    return m_entries.size();
}

} // namespace kairo
