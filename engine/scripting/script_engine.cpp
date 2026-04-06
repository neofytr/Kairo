#include "scripting/script_engine.h"
#include "core/log.h"
#include "ecs/world.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace kairo {

// store world pointer in lua registry so C functions can access it
static const char* WORLD_KEY = "kairo_world";
static const char* SCRIPT_ENGINE_KEY = "kairo_script_engine";

static World* get_world_from_lua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, WORLD_KEY);
    auto* w = static_cast<World*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return w;
}

// --- lua-callable engine functions ---

// kairo.log(message)
static int lua_kairo_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    log::info("[lua] %s", msg);
    return 0;
}

// kairo.create_entity() -> entity_id
static int lua_kairo_create_entity(lua_State* L) {
    World* world = get_world_from_lua(L);
    if (!world) return 0;

    Entity e = world->create();
    lua_pushinteger(L, static_cast<lua_Integer>(e.id));
    return 1;
}

// kairo.entity_count() -> count
static int lua_kairo_entity_count(lua_State* L) {
    World* world = get_world_from_lua(L);
    if (!world) return 0;

    lua_pushinteger(L, static_cast<lua_Integer>(world->entity_count()));
    return 1;
}

// --- ScriptEngine implementation ---

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::init() {
    m_lua = luaL_newstate();
    if (!m_lua) {
        log::error("script engine: failed to create lua state");
        return false;
    }

    luaL_openlibs(m_lua);
    bind_engine_api();

    log::info("script engine initialized (Lua %s)", LUA_VERSION);
    return true;
}

void ScriptEngine::shutdown() {
    if (m_lua) {
        lua_close(m_lua);
        m_lua = nullptr;
        log::info("script engine shut down");
    }
}

bool ScriptEngine::load_file(const std::string& path) {
    if (!m_lua) return false;

    // update world pointer in registry
    lua_pushlightuserdata(m_lua, m_world);
    lua_setfield(m_lua, LUA_REGISTRYINDEX, WORLD_KEY);

    if (luaL_loadfile(m_lua, path.c_str()) != LUA_OK) {
        log::error("script error loading '%s': %s", path.c_str(), lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
        log::error("script error running '%s': %s", path.c_str(), lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    log::info("loaded script: %s", path.c_str());
    return true;
}

bool ScriptEngine::execute(const std::string& code) {
    if (!m_lua) return false;

    lua_pushlightuserdata(m_lua, m_world);
    lua_setfield(m_lua, LUA_REGISTRYINDEX, WORLD_KEY);

    if (luaL_dostring(m_lua, code.c_str()) != LUA_OK) {
        log::error("script error: %s", lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::call(const std::string& func_name) {
    if (!m_lua) return false;

    lua_getglobal(m_lua, func_name.c_str());
    if (!lua_isfunction(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return false;
    }

    if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
        log::error("script error calling '%s': %s", func_name.c_str(), lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::call_update(u64 entity_id, float dt) {
    if (!m_lua) return false;

    lua_getglobal(m_lua, "on_update");
    if (!lua_isfunction(m_lua, -1)) {
        lua_pop(m_lua, 1);
        return false;
    }

    lua_pushinteger(m_lua, static_cast<lua_Integer>(entity_id));
    lua_pushnumber(m_lua, dt);

    if (lua_pcall(m_lua, 2, 0, 0) != LUA_OK) {
        log::error("script on_update error: %s", lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        return false;
    }

    return true;
}

void ScriptEngine::bind_engine_api() {
    // create the "kairo" table
    lua_newtable(m_lua);

    lua_pushcfunction(m_lua, lua_kairo_log);
    lua_setfield(m_lua, -2, "log");

    lua_pushcfunction(m_lua, lua_kairo_create_entity);
    lua_setfield(m_lua, -2, "create_entity");

    lua_pushcfunction(m_lua, lua_kairo_entity_count);
    lua_setfield(m_lua, -2, "entity_count");

    lua_setglobal(m_lua, "kairo");
}

} // namespace kairo
