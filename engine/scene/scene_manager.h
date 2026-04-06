#pragma once

#include "core/types.h"
#include <vector>
#include <memory>
#include <string>

namespace kairo {

class Scene;

// manages a stack of scenes
// - push: overlay a new scene on top (pauses current)
// - pop: remove top scene (resumes one below)
// - switch_to: replace entire stack with a new scene
class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    // take ownership of a scene and push it onto the stack
    void push(std::unique_ptr<Scene> scene);

    // pop the top scene off the stack
    void pop();

    // replace the entire stack with a new scene
    void switch_to(std::unique_ptr<Scene> scene);

    // process any pending transitions (call once per frame before update)
    void process_pending();

    // update/render the active scene
    void fixed_update(float dt);
    void update(float dt);
    void render();

    // current active scene (top of stack)
    Scene* get_active() const;

    bool is_empty() const { return m_stack.empty(); }
    size_t depth() const { return m_stack.size(); }

private:
    std::vector<std::unique_ptr<Scene>> m_stack;

    // deferred operations to avoid modifying the stack mid-update
    enum class PendingOp { None, Push, Pop, Switch };
    PendingOp m_pending_op = PendingOp::None;
    std::unique_ptr<Scene> m_pending_scene;
};

} // namespace kairo
