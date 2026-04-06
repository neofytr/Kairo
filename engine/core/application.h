#pragma once

#include "core/types.h"
#include "graphics/window.h"
#include "graphics/camera.h"
#include "ecs/world.h"

#include <string>

namespace kairo {

// user-facing base class
// inherit from this to create a game / demo
class Application {
public:
    virtual ~Application() = default;

    // override these in your game
    virtual void on_init() {}
    virtual void on_shutdown() {}

    // called at fixed rate (physics, gameplay)
    virtual void on_fixed_update(float dt) {}

    // called every frame (variable dt — animation, input response)
    virtual void on_update(float dt) {}

    // called every frame after update
    virtual void on_render() {}

    // access engine systems
    World& get_world() { return m_world; }
    Camera& get_camera() { return m_camera; }
    Window& get_window() { return *m_window; }

protected:
    World m_world;
    Camera m_camera;

private:
    friend class Engine;
    Window* m_window = nullptr; // set by Engine, not owned
};

} // namespace kairo
