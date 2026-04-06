#include "graphics/window.h"
#include "core/log.h"

#include <glad/glad.h>

int main() {
    kairo::log::info("starting Kairo engine");

    kairo::Window window;
    if (!window.init({ "Kairo Engine", 1280, 720, true })) {
        kairo::log::error("failed to initialize — exiting");
        return 1;
    }

    // main loop — just clear the screen for now
    while (!window.should_close()) {
        window.poll_events();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.swap_buffers();
    }

    kairo::log::info("shutting down");
    return 0;
}
