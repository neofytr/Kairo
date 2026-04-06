#pragma once

#include "core/application.h"
#include "scene/scene.h"
#include "scene/scene_serializer.h"
#include "graphics/texture.h"
#include "graphics/light.h"
#include "graphics/camera_controller.h"
#include "editor/editor_panels.h"
#include "physics/physics_world.h"
#include "physics/collision.h"
#include "physics/rigidbody.h"
#include "particles/particle_emitter.h"
#include "scripting/script_engine.h"
#include "audio/audio.h"
#include "graphics/post_process.h"
#include "graphics/effects/bloom.h"
#include "graphics/effects/vignette.h"
#include "graphics/shader_watcher.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

// --- game components ---

struct TransformComponent {
    kairo::Vec3 position;
    kairo::Vec3 scale = { 1.0f, 1.0f, 1.0f };
    float rotation = 0.0f;
};

struct SpriteComponent {
    kairo::Vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    kairo::Vec2 size  = { 32.0f, 32.0f };
    int layer = 100;
};

struct VelocityComponent {
    kairo::Vec3 velocity;
};

struct PlayerTag {};

struct BulletTag {
    float lifetime = 2.0f;
};

struct EnemyTag {
    float health = 1.0f;
};

struct WallTag {};

// --- scenes ---

class GameplayScene : public kairo::Scene {
public:
    GameplayScene(kairo::SceneManager& scenes);

    void on_enter() override;
    void on_exit() override;
    void on_fixed_update(float dt) override;
    void on_update(float dt) override;
    void on_render() override;
    void on_pause() override;
    void on_resume() override;

private:
    kairo::SceneManager& m_scenes;
    kairo::SceneSerializer m_serializer;
    kairo::PhysicsWorld m_physics;
    kairo::ParticleEmitter m_player_trail;
    kairo::ParticleEmitter m_hit_burst;
    kairo::ParticleEmitter m_muzzle_flash;
    kairo::LightSystem m_lights;
    kairo::CameraController m_cam_controller;
    kairo::ScriptEngine m_scripting;
    kairo::AudioSystem m_audio;
    kairo::PostProcessStack m_post_process;
    kairo::ShaderWatcher m_shader_watcher;
    kairo::Texture m_checkerboard;

    float m_player_speed = 350.0f;
    float m_shoot_cooldown = 0.0f;
    float m_time = 0.0f;
    int m_score = 0;
    int m_wave = 1;
    float m_wave_timer = 0.0f;

    void setup_serializer();
    void setup_particles();
    void spawn_player();
    void spawn_enemies(int count);
    void spawn_walls();
    void shoot_bullet(const kairo::Vec2& from, const kairo::Vec2& dir);
};

class PauseScene : public kairo::Scene {
public:
    PauseScene(kairo::SceneManager& scenes);

    void on_enter() override;
    void on_update(float dt) override;
    void on_render() override;

private:
    kairo::SceneManager& m_scenes;
    float m_time = 0.0f;
};

// --- application ---

class Game : public kairo::Application {
public:
    void on_init() override;
    void on_shutdown() override;
    void on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) override;

private:
    kairo::StatsPanel m_stats_panel;
    kairo::HierarchyPanel m_hierarchy_panel;
    kairo::InspectorPanel m_inspector_panel;

    void setup_inspector();
};
