#pragma once

#include "core/types.h"
#include "graphics/window.h"
#include "graphics/renderer.h"
#include "scene/scene_manager.h"

#include <string>

namespace kairo {

class Application {
public:
    virtual ~Application() = default;

    virtual void on_init() {}
    virtual void on_shutdown() {}

    virtual void on_fixed_update(float dt) {}
    virtual void on_update(float dt) {}
    virtual void on_render() {}

    // called when ImGui editor is visible — override to draw custom panels
    virtual void on_editor_ui(float fps, float dt, const Renderer::Stats& stats) {}

    SceneManager& get_scenes() { return m_scenes; }
    Window& get_window() { return *m_window; }

    bool using_scenes() const { return m_use_scenes; }

protected:
    SceneManager m_scenes;
    bool m_use_scenes = false;

private:
    friend class Engine;
    Window* m_window = nullptr;
};

} // namespace kairo
