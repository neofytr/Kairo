#include "graphics/shader_watcher.h"
#include "core/log.h"

namespace kairo {

void ShaderWatcher::watch(Shader* shader, const std::string& vert_path, const std::string& frag_path) {
    WatchEntry entry;
    entry.shader = shader;
    entry.vert_path = vert_path;
    entry.frag_path = frag_path;

    // grab initial timestamps
    try {
        entry.vert_time = std::filesystem::last_write_time(vert_path);
        entry.frag_time = std::filesystem::last_write_time(frag_path);
    } catch (const std::filesystem::filesystem_error& e) {
        log::warn("shader_watcher: failed to read timestamps: %s", e.what());
        return;
    }

    m_entries.push_back(std::move(entry));
}

void ShaderWatcher::check() {
    for (auto& entry : m_entries) {
        try {
            auto vert_now = std::filesystem::last_write_time(entry.vert_path);
            auto frag_now = std::filesystem::last_write_time(entry.frag_path);

            if (vert_now != entry.vert_time || frag_now != entry.frag_time) {
                // shader source changed, reload
                log::info("shader_watcher: reloading [%s, %s]",
                          entry.vert_path.c_str(), entry.frag_path.c_str());

                entry.shader->load(entry.vert_path, entry.frag_path);
                entry.vert_time = vert_now;
                entry.frag_time = frag_now;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            log::warn("shader_watcher: filesystem error: %s", e.what());
        }
    }
}

size_t ShaderWatcher::watched_count() const {
    return m_entries.size();
}

} // namespace kairo
