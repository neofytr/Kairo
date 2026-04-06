#include "core/engine.h"
#include "core/application.h"
#include "core/log.h"
#include "graphics/renderer.h"
#include "input/input.h"

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

    m_time.init();
    m_running = true;

    log::info("engine initialized successfully");
    return true;
}

void Engine::run(Application& app) {
    app.m_window = &m_window;
    app.on_init();

    while (m_running && !m_window.should_close()) {
        // start of frame
        Input::update();
        m_window.poll_events();
        m_time.update();

        // escape to quit (can be overridden by application)
        if (Input::is_key_pressed(Key::Escape)) {
            m_running = false;
            break;
        }

        // fixed timestep updates (physics, gameplay logic)
        float fixed_dt = m_time.fixed_delta_time();
        while (m_time.accumulator() >= fixed_dt) {
            app.on_fixed_update(fixed_dt);
            m_time.consume_accumulator(fixed_dt);
        }

        // variable update (animation, input-driven movement)
        app.on_update(m_time.delta_time());

        // render
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Renderer::reset_stats();
        app.on_render();

        m_window.swap_buffers();
    }

    app.on_shutdown();
}

void Engine::shutdown() {
    Renderer::shutdown();
    m_window.shutdown();
    log::info("engine shut down");
}

} // namespace kairo
