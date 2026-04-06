#include "core/engine.h"
#include "core/application.h"
#include "core/log.h"
#include "graphics/renderer.h"
#include "input/input.h"
#include "editor/imgui_layer.h"

#include <glad/glad.h>

namespace kairo {

bool Engine::init(const EngineConfig& config) {
    log::info("initializing Kairo engine");

    if (!m_window.init(config.window)) {
        return false;
    }

    Input::init(m_window.get_native());

    if (!Renderer::init()) {
        return false;
    }

    // ImGui chains to our existing callbacks, so Input still gets events
    ImGuiLayer::init(m_window.get_native());

    m_time.init();
    m_running = true;

    log::info("engine initialized successfully");
    return true;
}

void Engine::run(Application& app) {
    app.m_window = &m_window;
    app.on_init();

    while (m_running && !m_window.should_close()) {
        Input::update();
        m_window.poll_events();
        m_time.update();

        if (Input::is_key_pressed(Key::Escape)) {
            m_running = false;
            break;
        }

        // F1 toggles editor
        if (Input::is_key_pressed(Key::F1)) {
            ImGuiLayer::toggle();
        }

        // scene transitions
        if (app.using_scenes()) {
            app.get_scenes().process_pending();
        }

        // fixed timestep
        float fixed_dt = m_time.fixed_delta_time();
        while (m_time.accumulator() >= fixed_dt) {
            if (app.using_scenes()) {
                app.get_scenes().fixed_update(fixed_dt);
            } else {
                app.on_fixed_update(fixed_dt);
            }
            m_time.consume_accumulator(fixed_dt);
        }

        // variable update
        if (app.using_scenes()) {
            app.get_scenes().update(m_time.delta_time());
        } else {
            app.on_update(m_time.delta_time());
        }

        // render game
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Renderer::reset_stats();
        if (app.using_scenes()) {
            app.get_scenes().render();
        } else {
            app.on_render();
        }

        // render ImGui on top
        ImGuiLayer::begin_frame();
        if (ImGuiLayer::is_visible()) {
            app.on_editor_ui(m_time.fps(), m_time.delta_time(), Renderer::get_stats());
        }
        ImGuiLayer::end_frame();

        m_window.swap_buffers();
    }

    app.on_shutdown();
}

void Engine::shutdown() {
    ImGuiLayer::shutdown();
    Renderer::shutdown();
    m_window.shutdown();
    log::info("engine shut down");
}

} // namespace kairo
