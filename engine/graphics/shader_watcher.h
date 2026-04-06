#pragma once

#include "core/types.h"
#include "graphics/shader.h"
#include <string>
#include <vector>
#include <filesystem>

namespace kairo {

class ShaderWatcher {
public:
    void watch(Shader* shader, const std::string& vert_path, const std::string& frag_path);
    void check(); // call once per frame or periodically
    size_t watched_count() const;

private:
    struct WatchEntry {
        Shader* shader;
        std::string vert_path;
        std::string frag_path;
        std::filesystem::file_time_type vert_time;
        std::filesystem::file_time_type frag_time;
    };

    std::vector<WatchEntry> m_entries;
};

} // namespace kairo
