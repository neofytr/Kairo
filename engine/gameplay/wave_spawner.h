#pragma once
#include "core/types.h"
#include <vector>
#include <functional>
#include <string>

namespace kairo {

// defines what enemies spawn in a wave
struct WaveEntry {
    std::string type;       // enemy type identifier
    i32 count = 1;          // how many to spawn
    float delay = 0.0f;     // delay before this group spawns (from wave start)
};

struct WaveConfig {
    std::vector<WaveEntry> entries;
    float duration = 10.0f; // time before next wave starts (if all enemies dead or timer expires)
};

// callback: (enemy_type, spawn_index) — called for each enemy to spawn
using SpawnCallback = std::function<void(const std::string& type, i32 index)>;

class WaveSpawner {
public:
    void add_wave(const WaveConfig& wave) {
        m_waves.push_back(wave);
    }

    // auto-generate waves with increasing difficulty
    // formula: base_count + wave_number * scale, types cycle through provided list
    void generate_waves(i32 num_waves, i32 base_count, i32 scale_per_wave,
                       const std::vector<std::string>& enemy_types, float base_duration = 10.0f) {
        for (i32 w = 0; w < num_waves; ++w) {
            WaveConfig config;
            config.duration = base_duration + static_cast<float>(w) * 2.0f;

            i32 total = base_count + w * scale_per_wave;

            // distribute enemies across types with staggered delays
            for (i32 t = 0; t < static_cast<i32>(enemy_types.size()); ++t) {
                WaveEntry entry;
                entry.type = enemy_types[t % enemy_types.size()];
                entry.count = total / static_cast<i32>(enemy_types.size());
                entry.delay = static_cast<float>(t) * 1.0f;

                // give remainder to first type
                if (t == 0) {
                    entry.count += total % static_cast<i32>(enemy_types.size());
                }

                if (entry.count > 0) {
                    config.entries.push_back(entry);
                }
            }

            m_waves.push_back(config);
        }
    }

    void set_spawn_callback(SpawnCallback cb) {
        m_spawn_callback = std::move(cb);
    }

    void start() {
        m_current_wave = -1;
        m_active = true;
        m_complete = false;
        start_next_wave();
    }

    void update(float dt) {
        if (!m_active || m_complete) return;

        m_wave_timer += dt;

        // check for entries that should spawn based on delay
        auto& wave = m_waves[m_current_wave];
        for (i32 i = 0; i < static_cast<i32>(wave.entries.size()); ++i) {
            if (m_spawned[i]) continue;
            if (m_wave_timer >= wave.entries[i].delay) {
                m_spawned[i] = true;
                // spawn all enemies in this entry
                if (m_spawn_callback) {
                    for (i32 j = 0; j < wave.entries[i].count; ++j) {
                        m_spawn_callback(wave.entries[i].type, j);
                    }
                }
            }
        }

        // advance to next wave when duration expires
        if (m_wave_timer >= wave.duration) {
            start_next_wave();
        }
    }

    void reset() {
        m_current_wave = -1;
        m_wave_timer = 0.0f;
        m_spawn_timer = 0.0f;
        m_active = false;
        m_complete = false;
        m_spawned.clear();
    }

    i32 get_current_wave() const { return m_current_wave; }
    i32 get_total_waves() const { return static_cast<i32>(m_waves.size()); }
    bool is_complete() const { return m_complete; }
    bool is_wave_active() const { return m_active && !m_complete; }
    float get_wave_timer() const { return m_wave_timer; }

private:
    std::vector<WaveConfig> m_waves;
    SpawnCallback m_spawn_callback;

    i32 m_current_wave = -1;
    float m_wave_timer = 0.0f;
    float m_spawn_timer = 0.0f;
    bool m_active = false;
    bool m_complete = false;

    // track which entries have been spawned
    std::vector<bool> m_spawned;

    void start_next_wave() {
        ++m_current_wave;

        // all waves done
        if (m_current_wave >= static_cast<i32>(m_waves.size())) {
            m_active = false;
            m_complete = true;
            return;
        }

        // reset timer and spawned flags for new wave
        m_wave_timer = 0.0f;
        m_spawn_timer = 0.0f;
        m_spawned.assign(m_waves[m_current_wave].entries.size(), false);
    }
};

} // namespace kairo
