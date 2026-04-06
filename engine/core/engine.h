#pragma once

#include "core/types.h"
#include "core/time.h"
#include "graphics/window.h"

namespace kairo {

class Application;

struct EngineConfig {
    WindowConfig window;
};

class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    // initialize engine, returns false on failure
    bool init(const EngineConfig& config);

    // run the main loop with the given application
    void run(Application& app);

    // clean shutdown
    void shutdown();

private:
    Window m_window;
    Time m_time;
    bool m_running = false;
};

} // namespace kairo
