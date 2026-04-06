#pragma once

#include "core/types.h"
#include "graphics/window.h"
#include "scene/scene_manager.h"

#include <string>

namespace kairo {

// user-facing base class for the game
// can use scenes (via scene manager) or override update/render directly
class Application {
public:
    virtual ~Application() = default;

    virtual void on_init() {}
    virtual void on_shutdown() {}

    // direct update/render — used if you don't want scenes
    virtual void on_fixed_update(float dt) {}
    virtual void on_update(float dt) {}
    virtual void on_render() {}

    // scene management — push/switch scenes from your game code
    SceneManager& get_scenes() { return m_scenes; }
    Window& get_window() { return *m_window; }

    // if true, the engine delegates update/render to the scene manager
    // instead of calling on_update/on_render directly
    bool using_scenes() const { return m_use_scenes; }

protected:
    SceneManager m_scenes;
    bool m_use_scenes = false; // set to true in on_init if you want scene-based flow

private:
    friend class Engine;
    Window* m_window = nullptr;
};

} // namespace kairo
