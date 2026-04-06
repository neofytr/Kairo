#pragma once

#include "core/types.h"
#include "math/vec2.h"

#include <string>
#include <unordered_map>

// forward declare miniaudio types
struct ma_engine;
struct ma_sound;

namespace kairo {

// handle to a playing sound
using SoundHandle = u32;

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    bool init();
    void shutdown();

    // set the listener position (for spatial audio)
    void set_listener_position(const Vec2& pos);

    // master volume (0.0 - 1.0)
    void set_master_volume(float volume);

    // play a sound file — returns a handle for control
    SoundHandle play(const std::string& path, float volume = 1.0f, bool loop = false);

    // play a sound at a world position (spatial — attenuates with distance)
    SoundHandle play_at(const std::string& path, const Vec2& position,
                        float volume = 1.0f, float max_distance = 500.0f);

    // control a playing sound
    void stop(SoundHandle handle);
    void set_volume(SoundHandle handle, float volume);
    void set_looping(SoundHandle handle, bool loop);

    // stop all sounds
    void stop_all();

    // clean up finished sounds (call each frame)
    void update();

private:
    ma_engine* m_engine = nullptr;

    struct SoundInstance {
        ma_sound* sound = nullptr;
        bool spatial = false;
    };

    std::unordered_map<SoundHandle, SoundInstance> m_sounds;
    SoundHandle m_next_handle = 1;
};

} // namespace kairo
