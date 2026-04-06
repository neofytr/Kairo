#include "editor/profiler_panel.h"

#include <imgui.h>
#include <chrono>
#include <cstdio>
#include <algorithm>

namespace kairo {

// ---------- static storage ----------

std::unordered_map<std::string, ProfilerPanel::SectionData> ProfilerPanel::s_sections;

// ---------- ScopeTimer ----------

ScopeTimer::ScopeTimer(const char* name)
    : m_name(name)
    , m_start(std::chrono::high_resolution_clock::now())
{}

ScopeTimer::~ScopeTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(end - m_start).count();
    ProfilerPanel::record(m_name, ms);
}

// ---------- ProfilerPanel ----------

void ProfilerPanel::record(const std::string& name, float ms) {
    auto& s = s_sections[name];
    s.current_ms = ms;

    // push into rolling history
    s.history[s.index] = ms;
    s.index = (s.index + 1) % 60;

    // running average over the history buffer
    float sum = 0.0f;
    for (int i = 0; i < 60; ++i)
        sum += s.history[i];
    s.avg_ms = sum / 60.0f;

    // track peak
    s.max_ms = std::max(s.max_ms, ms);
}

void ProfilerPanel::clear() {
    for (auto& [name, s] : s_sections) {
        s.max_ms = 0.0f;
        std::fill(std::begin(s.history), std::end(s.history), 0.0f);
        s.avg_ms = 0.0f;
        s.index  = 0;
    }
}

void ProfilerPanel::draw(float fps, float dt) {
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Profiler")) {
        ImGui::End();
        return;
    }

    // push current frame time into rolling buffer
    float frame_ms = dt * 1000.0f;
    m_frame_times[m_frame_index] = frame_ms;
    m_frame_index = (m_frame_index + 1) % HISTORY_SIZE;

    // header: FPS and frame time
    ImGui::Text("FPS: %.1f  |  Frame: %.2f ms", fps, frame_ms);
    ImGui::Separator();

    // frame time graph (last 120 frames, circular buffer)
    // PlotLines expects a contiguous array; we pass offset so it reads correctly
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%.2f ms", frame_ms);
    ImGui::PlotLines("Frame##graph", m_frame_times, HISTORY_SIZE,
                     m_frame_index, overlay, 0.0f, 33.3f, ImVec2(0, 60));

    ImGui::Separator();

    // reset button
    if (ImGui::Button("Reset")) {
        clear();
        std::fill(std::begin(m_frame_times), std::end(m_frame_times), 0.0f);
    }

    ImGui::Separator();

    // section timings table
    if (ImGui::BeginTable("##sections", 5,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("Section");
        ImGui::TableSetupColumn("Current (ms)");
        ImGui::TableSetupColumn("Avg (ms)");
        ImGui::TableSetupColumn("Max (ms)");
        ImGui::TableSetupColumn("Sparkline");
        ImGui::TableHeadersRow();

        for (auto& [name, s] : s_sections) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", s.current_ms);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", s.avg_ms);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", s.max_ms);

            ImGui::TableSetColumnIndex(4);
            ImGui::PlotLines("##spark", s.history, 60, s.index,
                             nullptr, 0.0f, s.max_ms > 0.0f ? s.max_ms : 1.0f,
                             ImVec2(80, 20));
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace kairo
