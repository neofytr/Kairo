#include "editor/asset_browser.h"
#include "core/log.h"
#include <imgui.h>
#include <algorithm>

namespace kairo {

void AssetBrowser::set_root(const std::string& path) {
    m_root_path = path;
    m_current_path = path;
    m_selected.clear();
    m_needs_refresh = true;
}

void AssetBrowser::refresh() {
    m_entries.clear();
    m_needs_refresh = false;

    try {
        if (!std::filesystem::exists(m_current_path) ||
            !std::filesystem::is_directory(m_current_path)) {
            log::warn("asset_browser: path does not exist: %s", m_current_path.c_str());
            return;
        }

        for (auto& dir_entry : std::filesystem::directory_iterator(m_current_path)) {
            FileEntry fe;
            fe.name = dir_entry.path().filename().string();
            fe.full_path = dir_entry.path().string();
            fe.is_directory = dir_entry.is_directory();
            fe.extension = dir_entry.path().extension().string();
            m_entries.push_back(std::move(fe));
        }
    } catch (const std::filesystem::filesystem_error& e) {
        log::warn("asset_browser: filesystem error: %s", e.what());
        return;
    }

    // sort: directories first, then alphabetically by name
    std::sort(m_entries.begin(), m_entries.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.is_directory != b.is_directory) return a.is_directory > b.is_directory;
        return a.name < b.name;
    });
}

void AssetBrowser::draw() {
    if (m_needs_refresh) {
        refresh();
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Asset Browser")) {
        ImGui::End();
        return;
    }

    // breadcrumb: show current path
    ImGui::TextWrapped("Path: %s", m_current_path.c_str());
    ImGui::Separator();

    // ".." button to go up (only if not at root)
    if (m_current_path != m_root_path) {
        if (ImGui::Button("..")) {
            m_current_path = std::filesystem::path(m_current_path).parent_path().string();
            m_needs_refresh = true;
            m_selected.clear();
        }
    }

    // list entries
    for (auto& entry : m_entries) {
        draw_entry(entry);
    }

    // show selected file at the bottom
    ImGui::Separator();
    if (!m_selected.empty()) {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Selected: %s", m_selected.c_str());
    } else {
        ImGui::TextDisabled("No file selected");
    }

    ImGui::End();
}

void AssetBrowser::draw_entry(const FileEntry& entry) {
    ImVec4 color;

    if (entry.is_directory) {
        // yellow for directories
        color = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
    } else if (entry.extension == ".lua") {
        color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // green
    } else if (entry.extension == ".vert" || entry.extension == ".frag") {
        color = ImVec4(0.3f, 1.0f, 1.0f, 1.0f); // cyan
    } else if (entry.extension == ".png") {
        color = ImVec4(1.0f, 0.4f, 1.0f, 1.0f); // magenta
    } else if (entry.extension == ".wav") {
        color = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // orange
    } else {
        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
    }

    // build display label
    std::string label = entry.is_directory
        ? "[DIR] " + entry.name
        : entry.name;

    // highlight selected file
    bool is_selected = (!entry.is_directory && entry.full_path == m_selected);

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    if (ImGui::Selectable(label.c_str(), is_selected)) {
        if (entry.is_directory) {
            m_current_path = entry.full_path;
            m_needs_refresh = true;
            m_selected.clear();
        } else {
            m_selected = entry.full_path;
        }
    }
    ImGui::PopStyleColor();
}

} // namespace kairo
