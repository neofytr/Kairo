#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include <string>

// forward declare lua state
struct lua_State;

namespace kairo {

class World;

// wraps a Lua VM and exposes engine functionality to scripts
class ScriptEngine {
public:
    ScriptEngine() = default;
    ~ScriptEngine();

    bool init();
    void shutdown();

    // bind the ECS world so scripts can create/query entities
    void set_world(World* world) { m_world = world; }

    // load and execute a script file
    bool load_file(const std::string& path);

    // execute a lua string directly
    bool execute(const std::string& code);

    // call a global lua function by name
    bool call(const std::string& func_name);

    // call a per-entity update function: on_update(entity_id, dt)
    bool call_update(u64 entity_id, float dt);

    lua_State* get_state() const { return m_lua; }

private:
    lua_State* m_lua = nullptr;
    World* m_world = nullptr;

    void bind_engine_api();
};

// ECS component that attaches a script to an entity
struct ScriptComponent {
    std::string script_path;
    bool initialized = false;
};

} // namespace kairo
