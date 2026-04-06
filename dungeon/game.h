#pragma once

#include "core/application.h"
#include "scene/scene.h"
#include "graphics/texture.h"
#include "graphics/light.h"
#include "graphics/camera_controller.h"
#include "graphics/tilemap.h"
#include "graphics/ui.h"
#include "graphics/post_process.h"
#include "graphics/effects/bloom.h"
#include "graphics/effects/vignette.h"
#include "editor/editor_panels.h"
#include "editor/profiler_panel.h"
#include "physics/collision.h"
#include "physics/rigidbody.h"
#include "physics/physics_world.h"
#include "particles/particle_emitter.h"
#include "audio/audio.h"
#include "gameplay/health.h"
#include "gameplay/timer.h"
#include "gameplay/tween.h"
#include "gameplay/pathfinding.h"
#include "gameplay/state_machine.h"
#include "core/save_system.h"
#include "ecs/tag_system.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include <vector>
#include <random>

// --- components ---

struct Position {
    kairo::Vec3 pos;
    float rotation = 0.0f;
};

struct Sprite {
    kairo::Vec4 color = { 1, 1, 1, 1 };
    kairo::Vec4 base_color = { 1, 1, 1, 1 };
    kairo::Vec2 size = { 32, 32 };
    int layer = 100;
};

struct PlayerComp {
    int tile_x = 0, tile_y = 0;
    float move_cooldown = 0.0f;
    int keys_collected = 0;
    int potions = 0;
    int level = 1;
    int xp = 0;
};

enum class EnemyAI { Patrol, Chase, Idle };

struct EnemyComp {
    int tile_x = 0, tile_y = 0;
    float move_timer = 0.0f;
    float move_interval = 0.6f;
    float flash_timer = 0.0f;
    EnemyAI ai_state = EnemyAI::Patrol;
    int patrol_dir = 0;
};

struct ItemComp {
    enum Type { Key, Potion, Treasure, Exit };
    Type type = Key;
    int tile_x = 0, tile_y = 0;
    float bob_timer = 0.0f;
};

struct TrapComp {
    int tile_x = 0, tile_y = 0;
    float damage = 15.0f;
    bool armed = true;
    float rearm_timer = 0.0f;
};

// --- dungeon generation ---

struct Room {
    int x, y, w, h;
    int center_x() const { return x + w / 2; }
    int center_y() const { return y + h / 2; }
    bool intersects(const Room& other, int pad = 1) const {
        return x - pad < other.x + other.w && x + w + pad > other.x &&
               y - pad < other.y + other.h && y + h + pad > other.y;
    }
};

// --- scenes ---

class DungeonScene : public kairo::Scene {
public:
    DungeonScene(kairo::SceneManager& scenes, int floor = 1);

    void on_enter() override;
    void on_exit() override;
    void on_fixed_update(float dt) override;
    void on_update(float dt) override;
    void on_render() override;

private:
    kairo::SceneManager& m_scenes;
    kairo::Tilemap m_tilemap;
    kairo::NavGrid m_navgrid;
    kairo::LightSystem m_lights;
    kairo::CameraController m_cam_ctrl;
    kairo::AudioSystem m_audio;
    kairo::PostProcessStack m_post_fx;
    kairo::ParticleEmitter m_hit_particles;
    kairo::ParticleEmitter m_pickup_particles;
    kairo::ParticleEmitter m_trap_particles;
    kairo::TimerManager m_timers;
    kairo::TweenManager m_tweens;
    kairo::TagSystem m_tags;
    kairo::SaveSystem m_save;
    kairo::PhysicsWorld m_physics;

    int m_floor;
    int m_map_w = 40, m_map_h = 30;
    float m_tile_size = 32.0f;
    float m_time = 0.0f;
    int m_score = 0;
    bool m_game_over = false;
    float m_game_over_timer = 0.0f;
    std::string m_message;
    float m_message_timer = 0.0f;

    // tween targets
    float m_score_display = 0.0f;

    std::vector<Room> m_rooms;
    std::vector<std::vector<int>> m_grid; // 0=wall, 1=floor, 2=corridor
    std::mt19937 m_rng;

    void generate_dungeon();
    void carve_room(const Room& room);
    void carve_corridor(int x1, int y1, int x2, int y2);
    void populate_dungeon();
    void spawn_player(int tx, int ty);
    void spawn_enemy(int tx, int ty);
    void spawn_item(int tx, int ty, ItemComp::Type type);
    void spawn_trap(int tx, int ty);
    bool is_walkable(int tx, int ty) const;
    kairo::Vec2 tile_center(int tx, int ty) const;
    void show_message(const std::string& msg);
    void try_move_player(int dx, int dy);
    void next_floor();
};

class TitleScene : public kairo::Scene {
public:
    TitleScene(kairo::SceneManager& scenes);
    void on_enter() override;
    void on_update(float dt) override;
    void on_render() override;
private:
    kairo::SceneManager& m_scenes;
    float m_time = 0.0f;
};

class GameOverDungeonScene : public kairo::Scene {
public:
    GameOverDungeonScene(kairo::SceneManager& scenes, int score, int floor);
    void on_enter() override;
    void on_update(float dt) override;
    void on_render() override;
private:
    kairo::SceneManager& m_scenes;
    int m_score, m_floor;
    float m_time = 0.0f;
};

// --- application ---

class DungeonGame : public kairo::Application {
public:
    void on_init() override;
    void on_shutdown() override;
    void on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) override;
private:
    kairo::StatsPanel m_stats;
    kairo::ProfilerPanel m_profiler;
};
