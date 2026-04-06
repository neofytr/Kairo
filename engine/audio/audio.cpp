#include "audio/audio.h"
#include "core/log.h"

#include <miniaudio.h>
#include <vector>

namespace kairo {

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    m_engine = new ma_engine();

    ma_engine_config config = ma_engine_config_init();
    config.channels = 2;
    config.sampleRate = 44100;

    if (ma_engine_init(&config, m_engine) != MA_SUCCESS) {
        log::error("audio: failed to initialize miniaudio engine");
        delete m_engine;
        m_engine = nullptr;
        return false;
    }

    log::info("audio system initialized");
    return true;
}

void AudioSystem::shutdown() {
    if (!m_engine) return;

    stop_all();

    ma_engine_uninit(m_engine);
    delete m_engine;
    m_engine = nullptr;

    log::info("audio system shut down");
}

void AudioSystem::set_listener_position(const Vec2& pos) {
    if (!m_engine) return;
    // miniaudio uses 3D coords — we set y=0 and use x/z for 2D
    ma_engine_listener_set_position(m_engine, 0, pos.x, 0.0f, pos.y);
}

void AudioSystem::set_master_volume(float volume) {
    if (!m_engine) return;
    ma_engine_set_volume(m_engine, volume);
}

SoundHandle AudioSystem::play(const std::string& path, float volume, bool loop) {
    if (!m_engine) return 0;

    ma_sound* sound = new ma_sound();
    if (ma_sound_init_from_file(m_engine, path.c_str(), 0, nullptr, nullptr, sound) != MA_SUCCESS) {
        log::warn("audio: failed to load '%s'", path.c_str());
        delete sound;
        return 0;
    }

    ma_sound_set_volume(sound, volume);
    ma_sound_set_looping(sound, loop);
    ma_sound_set_spatialization_enabled(sound, MA_FALSE);
    ma_sound_start(sound);

    SoundHandle handle = m_next_handle++;
    m_sounds[handle] = { sound, false };
    return handle;
}

SoundHandle AudioSystem::play_at(const std::string& path, const Vec2& position,
                                  float volume, float max_distance) {
    if (!m_engine) return 0;

    ma_sound* sound = new ma_sound();
    if (ma_sound_init_from_file(m_engine, path.c_str(), 0, nullptr, nullptr, sound) != MA_SUCCESS) {
        log::warn("audio: failed to load '%s'", path.c_str());
        delete sound;
        return 0;
    }

    ma_sound_set_volume(sound, volume);
    ma_sound_set_spatialization_enabled(sound, MA_TRUE);
    ma_sound_set_position(sound, position.x, 0.0f, position.y);
    ma_sound_set_min_distance(sound, 1.0f);
    ma_sound_set_max_distance(sound, max_distance);
    ma_sound_set_attenuation_model(sound, ma_attenuation_model_linear);
    ma_sound_start(sound);

    SoundHandle handle = m_next_handle++;
    m_sounds[handle] = { sound, true };
    return handle;
}

void AudioSystem::stop(SoundHandle handle) {
    auto it = m_sounds.find(handle);
    if (it == m_sounds.end()) return;

    ma_sound_stop(it->second.sound);
    ma_sound_uninit(it->second.sound);
    delete it->second.sound;
    m_sounds.erase(it);
}

void AudioSystem::set_volume(SoundHandle handle, float volume) {
    auto it = m_sounds.find(handle);
    if (it != m_sounds.end()) {
        ma_sound_set_volume(it->second.sound, volume);
    }
}

void AudioSystem::set_looping(SoundHandle handle, bool loop) {
    auto it = m_sounds.find(handle);
    if (it != m_sounds.end()) {
        ma_sound_set_looping(it->second.sound, loop);
    }
}

void AudioSystem::stop_all() {
    for (auto& [handle, inst] : m_sounds) {
        ma_sound_stop(inst.sound);
        ma_sound_uninit(inst.sound);
        delete inst.sound;
    }
    m_sounds.clear();
}

void AudioSystem::update() {
    // clean up sounds that have finished playing
    std::vector<SoundHandle> finished;

    for (auto& [handle, inst] : m_sounds) {
        if (!ma_sound_is_playing(inst.sound) && !ma_sound_is_looping(inst.sound)) {
            finished.push_back(handle);
        }
    }

    for (auto h : finished) {
        auto& inst = m_sounds[h];
        ma_sound_uninit(inst.sound);
        delete inst.sound;
        m_sounds.erase(h);
    }
}

} // namespace kairo
