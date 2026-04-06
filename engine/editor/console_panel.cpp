#include "editor/console_panel.h"
#include "scripting/script_engine.h"
#include <imgui.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace kairo {

void ConsolePanel::draw(ScriptEngine* script_engine) {
    ImGui::Begin("Console");

    // clear button
    if (ImGui::Button("Clear")) {
        clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_auto_scroll);

    ImGui::Separator();

    // scrolling log region — reserve space for the input line
    const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("LogRegion", ImVec2(0, -footer_height), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : m_log) {
        // color lines by prefix
        if (line.rfind("[error]", 0) == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        } else if (line.rfind(">", 0) == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextUnformatted(line.c_str());
        }
    }

    // auto-scroll to bottom
    if (m_scroll_to_bottom || (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
    }
    m_scroll_to_bottom = false;

    ImGui::EndChild();

    ImGui::Separator();

    // input field
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    bool reclaim_focus = false;

    if (ImGui::InputText("##Input", m_input_buf, sizeof(m_input_buf), flags, history_callback, this)) {
        std::string cmd(m_input_buf);
        if (!cmd.empty()) {
            // add to history
            m_history.push_back(cmd);
            m_history_pos = -1;

            // echo command
            add_log("> %s", cmd.c_str());

            // execute
            bool ok = script_engine->execute(cmd);
            if (!ok) {
                add_log("[error] script execution failed");
            }
        }
        std::memset(m_input_buf, 0, sizeof(m_input_buf));
        reclaim_focus = true;
    }

    // keep focus on the input field
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus) {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void ConsolePanel::add_log(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    m_log.emplace_back(buf);

    // trim log to 500 lines
    if (m_log.size() > 500) {
        m_log.erase(m_log.begin(), m_log.begin() + static_cast<int>(m_log.size() - 500));
    }

    m_scroll_to_bottom = true;
}

void ConsolePanel::clear() {
    m_log.clear();
}

int ConsolePanel::history_callback(ImGuiInputTextCallbackData* cb_data) {
    auto* panel = static_cast<ConsolePanel*>(cb_data->UserData);

    if (panel->m_history.empty()) {
        return 0;
    }

    const int prev_pos = panel->m_history_pos;

    if (cb_data->EventKey == ImGuiKey_UpArrow) {
        if (panel->m_history_pos == -1) {
            panel->m_history_pos = static_cast<int>(panel->m_history.size()) - 1;
        } else if (panel->m_history_pos > 0) {
            panel->m_history_pos--;
        }
    } else if (cb_data->EventKey == ImGuiKey_DownArrow) {
        if (panel->m_history_pos != -1) {
            if (panel->m_history_pos < static_cast<int>(panel->m_history.size()) - 1) {
                panel->m_history_pos++;
            } else {
                panel->m_history_pos = -1;
            }
        }
    }

    if (prev_pos != panel->m_history_pos) {
        if (panel->m_history_pos >= 0) {
            const auto& hist = panel->m_history[static_cast<size_t>(panel->m_history_pos)];
            cb_data->DeleteChars(0, cb_data->BufTextLen);
            cb_data->InsertChars(0, hist.c_str());
        } else {
            cb_data->DeleteChars(0, cb_data->BufTextLen);
        }
    }

    return 0;
}

} // namespace kairo
