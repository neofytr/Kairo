#include "scene/scene_manager.h"
#include "scene/scene.h"
#include "core/log.h"

namespace kairo {

SceneManager::~SceneManager() {
    // clean up all scenes in reverse order
    while (!m_stack.empty()) {
        m_stack.back()->on_exit();
        m_stack.pop_back();
    }
}

void SceneManager::push(std::unique_ptr<Scene> scene) {
    m_pending_op = PendingOp::Push;
    m_pending_scene = std::move(scene);
}

void SceneManager::pop() {
    m_pending_op = PendingOp::Pop;
    m_pending_scene = nullptr;
}

void SceneManager::switch_to(std::unique_ptr<Scene> scene) {
    m_pending_op = PendingOp::Switch;
    m_pending_scene = std::move(scene);
}

void SceneManager::process_pending() {
    if (m_pending_op == PendingOp::None) return;

    switch (m_pending_op) {
    case PendingOp::Push:
        if (!m_stack.empty()) {
            m_stack.back()->on_pause();
        }
        log::info("scene push: %s", m_pending_scene->get_name().c_str());
        m_pending_scene->on_enter();
        m_stack.push_back(std::move(m_pending_scene));
        break;

    case PendingOp::Pop:
        if (!m_stack.empty()) {
            log::info("scene pop: %s", m_stack.back()->get_name().c_str());
            m_stack.back()->on_exit();
            m_stack.pop_back();
            if (!m_stack.empty()) {
                m_stack.back()->on_resume();
            }
        }
        break;

    case PendingOp::Switch:
        // tear down entire stack
        while (!m_stack.empty()) {
            m_stack.back()->on_exit();
            m_stack.pop_back();
        }
        log::info("scene switch: %s", m_pending_scene->get_name().c_str());
        m_pending_scene->on_enter();
        m_stack.push_back(std::move(m_pending_scene));
        break;

    default:
        break;
    }

    m_pending_op = PendingOp::None;
    m_pending_scene = nullptr;
}

void SceneManager::fixed_update(float dt) {
    if (auto* scene = get_active()) {
        scene->on_fixed_update(dt);
    }
}

void SceneManager::update(float dt) {
    if (auto* scene = get_active()) {
        scene->on_update(dt);
    }
}

void SceneManager::render() {
    if (auto* scene = get_active()) {
        scene->on_render();
    }
}

Scene* SceneManager::get_active() const {
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

} // namespace kairo
