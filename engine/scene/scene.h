#pragma once

#include "ecs/world.h"
#include "graphics/camera.h"
#include <string>

namespace kairo {

// base class for game scenes
// each scene owns its own ECS world and camera
class Scene {
public:
    Scene(const std::string& name) : m_name(name) {}
    virtual ~Scene() = default;

    // lifecycle hooks — override these in concrete scenes
    virtual void on_enter() {}    // called when scene becomes active
    virtual void on_exit() {}     // called when scene is deactivated
    virtual void on_pause() {}    // called when another scene is pushed on top
    virtual void on_resume() {}   // called when the scene above is popped

    virtual void on_fixed_update(float dt) {}
    virtual void on_update(float dt) {}
    virtual void on_render() {}

    const std::string& get_name() const { return m_name; }
    World& get_world() { return m_world; }
    Camera& get_camera() { return m_camera; }

protected:
    std::string m_name;
    World m_world;
    Camera m_camera;
};

} // namespace kairo
