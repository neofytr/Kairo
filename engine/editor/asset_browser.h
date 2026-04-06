#pragma once

#include "core/types.h"
#include <string>
#include <vector>
#include <filesystem>

namespace kairo {

class AssetBrowser {
public:
    void set_root(const std::string& path);
    void draw();

    // get the currently selected file path (empty if none)
    const std::string& get_selected() const { return m_selected; }
    bool has_selection() const { return !m_selected.empty(); }

private:
    std::string m_root_path = "assets";
    std::string m_current_path = "assets";
    std::string m_selected;

    struct FileEntry {
        std::string name;
        std::string full_path;
        bool is_directory;
        std::string extension;
    };

    std::vector<FileEntry> m_entries;
    bool m_needs_refresh = true;

    void refresh();
    void draw_entry(const FileEntry& entry);
};

} // namespace kairo
