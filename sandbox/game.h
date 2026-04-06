#pragma once

#include "core/application.h"
#include "scene/scene.h"
#include "scene/scene_serializer.h"
#include "graphics/texture.h"
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
    int layer = 100; // RenderLayer::Default
};

struct VelocityComponent {
    kairo::Vec3 velocity;
};

struct PlayerTag {};

// --- scenes ---

class GameplayScene : public kairo::Scene {
public:
    GameplayScene(kairo::SceneManager& scenes);

    void on_enter() override;
    void on_fixed_update(float dt) override;
    void on_update(float dt) override;
    void on_render() override;
    void on_pause() override;
    void on_resume() override;

private:
    kairo::SceneManager& m_scenes;
    kairo::SceneSerializer m_serializer;
    kairo::Texture m_checkerboard;
    float m_player_speed = 350.0f;
    float m_time = 0.0f;

    void setup_serializer();
};

class PauseScene : public kairo::Scene {
public:
    PauseScene(kairo::SceneManager& scenes);

    void on_enter() override;
    void on_fixed_update(float dt) override;
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
};
