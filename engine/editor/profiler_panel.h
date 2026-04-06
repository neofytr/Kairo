#pragma once

#include "core/types.h"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace kairo {

// Simple scope timer -- records elapsed time for a named section
class ScopeTimer {
public:
    ScopeTimer(const char* name);
    ~ScopeTimer();

private:
    const char* m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

// Stores profiling data and renders the ImGui panel
class ProfilerPanel {
public:
    // Record a timing for a named section (in milliseconds)
    static void record(const std::string& name, float ms);

    // Draw the ImGui profiler window
    void draw(float fps, float dt);

    // Reset all recorded data
    static void clear();

private:
    // Rolling frame time history for the graph
    static constexpr int HISTORY_SIZE = 120;
    float m_frame_times[HISTORY_SIZE] = {};
    int m_frame_index = 0;

    // Per-section timing data
    struct SectionData {
        float current_ms = 0.0f;
        float avg_ms     = 0.0f;
        float max_ms     = 0.0f;
        float history[60] = {};
        int   index       = 0;
    };
    static std::unordered_map<std::string, SectionData> s_sections;
};

// Convenience macro -- unique variable name per line
#define KAIRO_PROFILE_SCOPE(name) kairo::ScopeTimer _timer_##__LINE__(name)

} // namespace kairo
