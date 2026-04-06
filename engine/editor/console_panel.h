#pragma once

#include "core/types.h"
#include <imgui.h>
#include <string>
#include <vector>

namespace kairo {

class ScriptEngine;

class ConsolePanel {
public:
    void draw(ScriptEngine* script_engine);
    void add_log(const char* fmt, ...);
    void clear();

private:
    char m_input_buf[512] = {};
    std::vector<std::string> m_log;
    std::vector<std::string> m_history;
    int m_history_pos = -1;
    bool m_scroll_to_bottom = false;
    bool m_auto_scroll = true;

    static int history_callback(ImGuiInputTextCallbackData* data);
};

} // namespace kairo
