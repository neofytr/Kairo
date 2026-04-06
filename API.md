# Kairo Engine API Reference

## 1. Quick Start

```cpp
#include "core/engine.h"
#include "core/application.h"
#include "graphics/renderer.h"
#include "graphics/camera.h"
#include "ecs/world.h"

struct Transform { kairo::Vec3 position; float rotation = 0.0f; };
struct Sprite    { kairo::Vec4 color; kairo::Vec2 size; };

class MyGame : public kairo::Application {
    kairo::World m_world;
    kairo::Camera m_camera;

    void on_init() override {
        m_camera.set_orthographic(1280.0f, 720.0f);
        auto e = m_world.create();
        m_world.add_component(e, Transform{{ 0, 0, 0 }});
        m_world.add_component(e, Sprite{{ 1, 1, 1, 1 }, { 32, 32 }});
    }
    void on_update(float dt) override {
        m_world.query<Transform, Sprite>([&](kairo::Entity, Transform& t, Sprite&) {
            t.position.x += 50.0f * dt;
        });
    }
    void on_render() override {
        kairo::Renderer::begin(m_camera);
        m_world.query<Transform, Sprite>([](kairo::Entity, Transform& t, Sprite& s) {
            kairo::Renderer::draw_quad(t.position, s.size, t.rotation, s.color);
        });
        kairo::Renderer::end();
    }
};

int main() {
    kairo::Engine engine;
    if (!engine.init({ { "My Game", 1280, 720, true } })) return 1;
    MyGame game;
    engine.run(game);
    engine.shutdown();
}
```

---

## 2. Engine Core

### Engine

`Engine::init(EngineConfig)` creates the window and renderer. `Engine::run(Application&)` blocks until the window closes. `Engine::shutdown()` tears everything down.

`EngineConfig` wraps a `WindowConfig { string title, i32 width, i32 height, bool vsync }`.

### Application

Override these lifecycle methods:

- `on_init()` / `on_shutdown()` -- startup and teardown
- `on_fixed_update(float dt)` -- fixed timestep (60 Hz default), use for physics
- `on_update(float dt)` -- variable timestep, gameplay and rendering logic
- `on_render()` -- draw everything
- `on_editor_ui(fps, dt, stats)` -- ImGui panels when editor is visible

Accessors: `get_window()`, `get_scenes()`. Set `m_use_scenes = true` to enable scene management.

### Time

Managed by Engine. `delta_time()` is variable frame time, `fixed_delta_time()` defaults to 1/60s, `elapsed()` is total time, `fps()` is smoothed framerate.

### Logging

```cpp
kairo::log::info("wave %d started", wave_number);
kairo::log::warn("low hp: %.1f", hp);
kairo::log::error("file not found: %s", path);
kairo::log::min_level = kairo::log::Level::Warn; // suppress trace/info
```

### Type Aliases (`core/types.h`)

`u8`..`u64`, `i8`..`i64`, `f32`, `f64`, `Unique<T>` (`unique_ptr`), `Shared<T>` (`shared_ptr`).

---

## 3. ECS

Archetype-based. Entities are 64-bit handles (32 index + 32 generation). Components are plain structs.

### World

```cpp
kairo::World world;
Entity e = world.create();
world.destroy(e);
bool alive = world.is_alive(e);

world.add_component(e, MyComp{ 42 });
auto& c = world.get_component<MyComp>(e);
bool has = world.has_component<MyComp>(e);
world.remove_component<MyComp>(e);

// query: iterate all entities matching component types
world.query<Transform, Sprite>([](Entity e, Transform& t, Sprite& s) { ... });

// iterate all living entities (no filter)
world.for_each_entity([](Entity e) { ... });
```

**Gotcha:** Never destroy entities inside a `query` callback. Collect into a vector and destroy after:

```cpp
std::vector<Entity> dead;
world.query<BulletTag>([&](Entity e, BulletTag& b) {
    if (b.lifetime <= 0) dead.push_back(e);
});
for (auto e : dead) if (world.is_alive(e)) world.destroy(e);
```

### System

Optional base class: override `init(World&)`, `update(World&, float dt)`, `render(World&)`.

---

## 4. Math

### Vectors

`Vec2`, `Vec3`, `Vec4` with standard operators (`+`, `-`, `*`, `/`, `+=`, `-=`, `*=`, unary `-`).

Key methods: `length()`, `length_sq()`, `normalized()`, `dot()`. Vec2 adds `perp()` (90-deg CCW). Vec3 adds `cross()`. Vec4 constructs from `Vec3 + w`.

### Mat4

Column-major 4x4. Static factories:

```cpp
Mat4::identity()
Mat4::translate(Vec3), Mat4::scale(Vec3), Mat4::rotate_z(radians)
Mat4::rotate(radians, axis)
Mat4::ortho(left, right, bottom, top, near, far)
Mat4::perspective(fov_rad, aspect, near, far)
Mat4::look_at(eye, target, up)
```

`data()` returns `const float*` for OpenGL uniforms.

### Utilities (`math_utils.h`)

`radians()`, `degrees()`, `clamp()`, `lerp()`, `smoothstep()`, `approx_equal()`. Constants: `PI`, `TAU`, `DEG_TO_RAD`, `RAD_TO_DEG`.

---

## 5. Rendering

### Renderer

Static batch renderer. Draw calls between `begin`/`end` are sorted by layer and flushed.

```cpp
Renderer::begin(camera);
Renderer::set_layer(RenderLayer::Background); // 0, Default=100, Foreground=200, UI=300
Renderer::set_layer(42);                      // or custom int

// colored quads
Renderer::draw_quad(Vec3 pos, Vec2 size, Vec4 color);
Renderer::draw_quad(Vec3 pos, Vec2 size, float rotation, Vec4 color);

// textured quads
Renderer::draw_quad(Vec3 pos, Vec2 size, Texture& tex, Vec4 tint = white);
Renderer::draw_quad(Vec3 pos, Vec2 size, float rotation, Texture& tex, Vec2 uv_min, Vec2 uv_max, Vec4 tint);

// text
Renderer::draw_text("Score 100", Vec2{10, 20}, 2.0f, Vec4{1,1,1,1});
Renderer::draw_text(custom_font, "text", pos, scale, color);

Renderer::end();
auto stats = Renderer::get_stats(); // .draw_calls, .quad_count
```

**Gotcha:** For multiple passes (scene + HUD), use separate `begin`/`end` pairs with different cameras. The sandbox does this: scene camera for gameplay, then a fixed HUD camera.

### Camera

```cpp
Camera cam;
cam.set_orthographic(1280.0f, 720.0f);            // 2D
cam.set_perspective(fov_degrees, aspect, 0.1f, 100.0f); // 3D
cam.set_position(Vec3{100, 200, 0});
cam.set_rotation(z_radians);
```

### CameraController

Smooth-follow with dead zone, zoom, and trauma-based screen shake.

```cpp
CameraController ctrl;
ctrl.set_camera(&camera);
ctrl.set_smoothing(10.0f);
ctrl.set_dead_zone({20, 20});
ctrl.set_zoom(1.0f);          // 0.5 = 2x zoom in
ctrl.add_trauma(0.5f);        // shake, 0-1, decays automatically
ctrl.set_shake_intensity(10.0f, 0.03f);
ctrl.set_target(player_pos);
ctrl.update(dt);
```

### Texture

```cpp
Texture tex;
tex.load("assets/player.png"); // Nearest filter, ClampToEdge, flip_y by default
tex.load("assets/bg.png", { TextureFilter::Linear, TextureWrap::Repeat, true });
```

Non-copyable, move-only. `get_width()`, `get_height()`, `get_id()`.

### Font

```cpp
Font font;
font.load_bmfont("assets/fonts/my.fnt");
float w = font.measure_width("hello", 2.0f);
Font& builtin = Renderer::get_default_font();
```

### Sprite Sheets

```cpp
SpriteSheet sheet;
sheet.load(texture, 16, 16); // cell width/height in pixels
SpriteRegion r = sheet.get_region(col, row); // or get_region(flat_index)

SpriteAnimation anim;
anim.start_frame = 0; anim.frame_count = 8; anim.frame_duration = 0.1f;
anim.update(dt);
auto region = sheet.get_region(anim.get_sheet_index());
```

---

## 6. Input

Static polling. Call `is_key_pressed` for single-frame detection, `is_key_held` for continuous.

```cpp
Input::is_key_pressed(Key::Space);   // true on press frame only
Input::is_key_held(Key::W);          // true while held
Input::is_key_released(Key::Escape);

Input::is_mouse_pressed(MouseButton::Left);
Input::get_mouse_position();  // Vec2
Input::get_mouse_delta();     // Vec2
Input::get_scroll_delta();    // float
```

Key codes: `W/A/S/D`, `Space`, `Escape`, `Enter`, `Up/Down/Left/Right`, `F1`-`F12`, `LeftShift`, `LeftCtrl`.

---

## 7. Physics

### Colliders and Layers

```cpp
ColliderComponent col;
col.shape = ColliderComponent::Shape::Circle; // or AABB
col.radius = 14.0f;                           // circle
col.half_size = {16, 16};                     // AABB half-extents
col.offset = {0, 0};
col.is_trigger = false;                       // triggers detect but don't resolve

// bitmask layers: only collide if (a.layer & b.mask) && (b.layer & a.mask)
col.layer = CollisionLayer::Player;
col.mask  = CollisionLayer::Enemy | CollisionLayer::Wall;
```

Built-in: `None`, `Default`, `Player`, `Enemy`, `Bullet`, `Wall`, `Pickup`, `All`.

### RigidBody

```cpp
RigidBodyComponent rb;
rb.set_mass(3.0f);       // auto-computes inv_mass
rb.restitution = 0.3f;   // bounciness 0-1
rb.make_static();         // infinite mass, won't move
rb.apply_force(Vec2{0, -980});
```

### PhysicsWorld

You collect bodies from ECS, pass to `step`, then write positions back:

```cpp
PhysicsWorld physics;
physics.set_gravity({0, 0});

// in on_fixed_update:
std::vector<PhysicsBody> bodies;
world.query<Transform, ColliderComponent>([&](Entity e, Transform& t, ColliderComponent& col) {
    PhysicsBody b;
    b.entity = e; b.position = {t.position.x, t.position.y}; b.collider = col;
    if (world.has_component<RigidBodyComponent>(e))
        b.rigidbody = &world.get_component<RigidBodyComponent>(e);
    bodies.push_back(b);
});
physics.step(bodies, dt);

for (auto& b : bodies) {
    auto& t = world.get_component<Transform>(b.entity);
    t.position.x = b.position.x;
    t.position.y = b.position.y;
}
for (auto& c : physics.get_collisions()) { /* c.a, c.b, c.manifold */ }
```

### Raycasting

```cpp
Ray2D ray{ origin, direction.normalized() };
auto hit = ray_vs_aabb(ray, box);
auto hit = ray_vs_circle(ray, center, radius);
auto hit = raycast(ray, targets_vec, max_dist); // closest hit from RayTarget list
if (hit) { hit->point; hit->normal; hit->distance; hit->entity; }
```

### Joints

```cpp
JointSystem joints;
joints.add_distance_joint(a, b, rest_length);
joints.add_spring_joint(a, b, rest_length, stiffness, damping);
joints.solve(body_states, dt); // call during physics step
```

---

## 8. Particles

```cpp
ParticleEmitterConfig config;
config.max_particles = 600;
config.emit_rate = 80.0f;               // per second; 0 for burst-only
config.lifetime = {0.15f, 0.5f};        // Range<float>: random between min/max
config.velocity = {{-50,-50}, {50,50}};
config.start_size = {2.0f, 4.0f};
config.end_size = {0.0f, 0.5f};
config.start_color = {{1,1,1,1}, {1,1,0.5f,1}};
config.end_color = {{1,0,0,0}, {1,0.3f,0,0}};
config.damping = 4.0f;
config.looping = true;

ParticleEmitter emitter(config);
emitter.set_position(pos);
emitter.burst(12);    // instant burst
emitter.start();      // begin continuous
emitter.stop();
emitter.update(dt);
emitter.render(camera);
```

For one-shot effects (hit sparks, explosions): set `emit_rate = 0`, `looping = false`, then call `burst(count)` on impact.

---

## 9. Lighting

2D point light system. Renders a fullscreen pass multiplied over the scene.

```cpp
LightSystem lights;
lights.init();
lights.set_ambient({0.08f, 0.06f, 0.12f}, 0.15f);

// each frame:
lights.clear_lights();
lights.add_light({ position, color, intensity, radius }); // up to 32
lights.render(camera); // after Renderer::end(), before post-processing output
lights.shutdown();
```

---

## 10. Post-Processing

```cpp
PostProcessStack post;
post.init(1280, 720);
auto bloom = std::make_unique<BloomEffect>();
bloom->set_threshold(0.5f); bloom->set_intensity(0.35f);
post.add_effect(std::move(bloom));
auto vig = std::make_unique<VignetteEffect>();
vig->set_intensity(0.6f); vig->set_softness(0.55f);
post.add_effect(std::move(vig));

// in on_render:
post.begin_capture();       // redirect rendering to FBO
Renderer::begin(camera); /* draw scene */ Renderer::end();
lights.render(camera);
post.end_and_apply();       // run effect chain, blit to screen
// HUD goes after this with a separate begin/end pass
```

Custom effects: subclass `PostProcessEffect`, implement `init`, `shutdown`, `apply(u32 input_tex, Framebuffer& output)`.

---

## 11. Audio

```cpp
AudioSystem audio;
audio.init();
audio.set_master_volume(0.6f);

SoundHandle h = audio.play("assets/sounds/shoot.wav", 0.25f, false);
SoundHandle h = audio.play_at("assets/sounds/hit.wav", world_pos, 0.5f, 500.0f); // spatial

audio.stop(h); audio.set_volume(h, 0.8f); audio.set_looping(h, true); audio.stop_all();
audio.set_listener_position(player_pos);
audio.update(); // call each frame, cleans up finished sounds
audio.shutdown();
```

---

## 12. Scenes

`Scene` owns a `World` and `Camera`. Override: `on_enter`, `on_exit`, `on_pause`, `on_resume`, `on_fixed_update`, `on_update`, `on_render`.

`SceneManager` is a stack:

```cpp
scenes.push(std::make_unique<PauseScene>(scenes));   // overlay, pauses current
scenes.pop();                                         // resume previous
scenes.switch_to(std::make_unique<GameplayScene>()); // replace entire stack
```

Transitions are deferred -- applied on next `process_pending()`. Enable in Application: `m_use_scenes = true`.

### Scene Serialization

```cpp
SceneSerializer ser;
ser.register_component<Transform>("transform", to_json_fn, from_json_fn);
ser.save(world, "level.json");
ser.load(world, "level.json");
```

---

## 13. Tilemap

```cpp
Tilemap tilemap;
tilemap.init(10, 10, 80.0f); // grid width, height, world-space tile size
tilemap.set_tileset(&texture, {16, 16}); // pixel cell size
tilemap.set_tile(x, y, tile_id);
tilemap.set_tile(x, y, tile_id, tint);
tilemap.fill(tile_id); tilemap.clear();

tilemap.set_solid_tiles({0, 1, 5});
bool blocked = tilemap.is_solid(x, y);
Vec2 wp = tilemap.tile_to_world(x, y);
tilemap.world_to_tile(world_pos, tx, ty);

tilemap.render(camera_pos, view_size);         // textured
tilemap.render_colored(camera_pos, view_size); // colored quads (debug)
```

---

## 14. Gameplay Systems

### Health and Invincibility

```cpp
HealthComponent hp{100.0f, 100.0f};
hp.damage(25); hp.heal(10); hp.is_alive(); hp.ratio(); // 0-1

InvincibilityComponent inv;
inv.duration = 1.0f;
inv.activate(); inv.update(dt); inv.is_active();
```

### Steering Behaviors

Return force vectors. Apply to velocity manually.

```cpp
SteeringAgent agent{position, velocity, max_speed, max_force};
Vec2 f = steering::seek(agent, target);
Vec2 f = steering::flee(agent, threat);
Vec2 f = steering::arrive(agent, target, slow_radius);
Vec2 f = steering::pursue(agent, target_pos, target_vel);
Vec2 f = steering::wander(agent, wander_angle);
Vec2 f = steering::separation(agent, neighbors, desired_dist);
Vec2 f = steering::cohesion(agent, neighbors);
Vec2 f = steering::alignment(agent, neighbor_vels);
f = steering::truncate(f, max_force);
```

### Pathfinding

```cpp
NavGrid nav(50, 50, 32.0f);
nav.set_walkable(5, 3, false);
std::vector<Vec2> path = nav.find_path(start, end); // A*, world-space waypoints
```

### Animation State Machine

```cpp
AnimationStateMachine anim;
anim.add_state(AnimationClip{"idle", {0,1,2,3}, 0.15f, true});
anim.add_state(AnimationClip{"run", {4,5,6,7}, 0.1f, true});
anim.add_transition("idle", "run", [&]{ return speed > 0.1f; });
anim.set_state("idle");
anim.update(dt);
i32 frame = anim.get_current_frame();
```

### Timers and Tweens

```cpp
TimerManager timers;
timers.after(2.0f, []{ /* once */ });
timers.every(10.0f, []{ /* repeating */ });
timers.update(dt);

TweenManager tweens;
tweens.tween(&value, target, duration, ease::out_elastic, on_complete);
tweens.update(dt);
```

Easing functions: `ease::linear`, `in_quad`, `out_quad`, `in_out_quad`, `in_cubic`, `out_cubic`, `in_out_cubic`, `in_elastic`, `out_elastic`, `out_bounce`.

### Wave Spawner

```cpp
WaveSpawner spawner;
spawner.generate_waves(100, 4, 2, {"chaser","tank","swarmer"}, 8.0f);
spawner.set_spawn_callback([](const std::string& type, i32 index) { /* spawn */ });
spawner.start();
spawner.update(dt);
```

### Entity Pool

Reuses entities to avoid create/destroy churn.

```cpp
EntityPool pool;
pool.init(world, 50, [](World& w, Entity e) {
    w.add_component(e, BulletComponent{});
});
Entity e = pool.acquire();
pool.release(e);
```

Inactive entities have `PooledInactive` tag -- check and skip in queries.

---

## 15. Events

Type-safe publish/subscribe.

```cpp
struct DamageEvent { u64 target; float amount; };

EventBus bus;
u32 sub = bus.subscribe<DamageEvent>([](const DamageEvent& e) { /* handle */ });
bus.publish(DamageEvent{entity.id, 25.0f});

bus.queue(event);  // deferred
bus.flush();       // process queued
bus.unsubscribe(sub);
```

Built-in: `CollisionEvent`, `EntityDestroyedEvent`, `SceneChangedEvent`, `WindowResizedEvent`.

---

## 16. ECS Extensions

### Hierarchy

```cpp
HierarchySystem hier;
hier.attach(world, child, parent);
hier.detach(world, child);
Entity p = hier.get_parent(world, entity);
const auto& children = hier.get_children(world, entity);
```

Child transforms are not automatic. Use `get_parent` to read parent position and offset manually.

### Tags

```cpp
TagSystem tags;
tags.tag(world, entity, "enemy");
tags.untag(world, entity, "enemy");
bool b = tags.has_tag(world, entity, "enemy");
auto enemies = tags.find_by_tag(world, "enemy");
Entity p = tags.find_first(world, "player");
```

### Prefabs

```cpp
PrefabRegistry prefabs;
prefabs.register_component<Transform>("transform", to_json, from_json, has_check);
prefabs.save_prefab(world, entity, "enemy.json");
Entity e = prefabs.instantiate_from_file(world, "enemy.json");
prefabs.store("bullet", json_data);
Entity e = prefabs.instantiate_by_name(world, "bullet");
```

---

## 17. Editor

Toggle with F1. Built on Dear ImGui.

```cpp
// in on_editor_ui:
StatsPanel stats;     stats.draw(fps, dt, render_stats, entity_count);
HierarchyPanel hier;  hier.draw(world); Entity sel = hier.get_selected();
InspectorPanel insp;  insp.draw(world, selected_entity);
ProfilerPanel prof;   prof.draw(fps, dt);
ConsolePanel console; console.draw(&script_engine);
```

**InspectorPanel**: register component editors via `register_component(name, draw_func)`.

**Profiler**: use `KAIRO_PROFILE_SCOPE("name")` in code, results appear in the panel.

**Gizmo**: `gizmo.update(entity_pos, mouse_world, mouse_down, just_pressed, delta_out)` returns true while dragging. Modes: `Translate`, `Rotate`, `Scale`.

**Undo/Redo**: `CommandHistory` with `execute(unique_ptr<Command>)`, `undo()`, `redo()`. Use `LambdaCommand` for quick inline commands.

**Asset Browser**: `AssetBrowser::set_root("assets")`, `draw()`, `get_selected()`.

---

## 18. Scripting (Lua)

```cpp
ScriptEngine scripts;
scripts.init();
scripts.set_world(&world);
scripts.load_file("assets/scripts/ai.lua");
scripts.execute("print('hello')");
scripts.call("on_start");
scripts.call_update(entity.id, dt);
```

Attach to entities: `world.add_component(e, ScriptComponent{"assets/scripts/ai.lua"})`.

Hot reload: `ScriptHotReload::watch(path, &engine)`, call `check()` each frame.

---

## 19. Memory Allocators

**PoolAllocator<T>** -- fixed-size, O(1) alloc/free, auto-grows in chunks:

```cpp
PoolAllocator<Particle> pool(1024);
Particle* p = pool.allocate(args...);
pool.deallocate(p);
```

**LinearAllocator** -- bump allocator, no individual free, `reset()` frees all. Ideal for per-frame scratch:

```cpp
LinearAllocator alloc(64 * 1024);
auto* obj = alloc.create<MyStruct>(args...);
alloc.reset(); // free everything
```

**StackAllocator** -- LIFO with markers:

```cpp
StackAllocator stack(32 * 1024);
auto mark = stack.get_marker();
auto* a = stack.create<Foo>();
stack.free_to_marker(mark); // frees a
```

---

## 20. Advanced

### SIMD (`math/simd.h`)

SSE-accelerated with scalar fallbacks: `simd::add`, `simd::mul`, `simd::dot`, `simd::mat4_multiply`, `simd::mat4_transform`, `simd::mat4_transform_batch`.

### Job System

```cpp
JobSystem jobs;
jobs.init(0); // 0 = hardware_concurrency - 1
auto future = jobs.submit([]{ /* work */ });
jobs.parallel_for(1000, [](u32 i) { /* per-index */ });
jobs.wait_idle();
jobs.shutdown();
```

### Frame Graph

Declare render passes with dependencies. Manages FBOs and execution order.

```cpp
FrameGraph graph;
graph.add_pass({"scene", {}, "scene_color", execute_fn, {1280,720,false}});
graph.add_pass({"blur", {"scene_color"}, "blur_out", execute_fn});
graph.add_screen_pass("final", {"blur_out"}, execute_fn);
graph.compile(); graph.execute();
```

### Networking

```cpp
UDPSocket sock;
sock.bind(7777);
sock.send("127.0.0.1", 7778, data, size);
Packet pkt; if (sock.receive(pkt)) { /* pkt.data, pkt.sender_address */ }

NetEntityState state{id, pos, vel, rot};
serialize_state(state, bytes); deserialize_state(data, size, out);
```

### Shadows

```cpp
ShadowSystem shadows;
shadows.init();
shadows.add_caster({aabb});
shadows.render_shadows(light_pos, light_radius, camera, alpha);
```

### Debug Draw

```cpp
DebugDraw::set_enabled(true);
DebugDraw::line(a, b, color); DebugDraw::rect(center, size, color);
DebugDraw::circle(center, radius, color, segments);
DebugDraw::arrow(from, to, color);
DebugDraw::render(camera); // after scene, before HUD
```

### Shader Hot Reload

```cpp
ShaderWatcher watcher;
watcher.watch(&shader, "vert.glsl", "frag.glsl");
watcher.check(); // each frame
```

### Screen Coordinate Conversion

```cpp
Vec2 world = screen_to_world(screen_pos, camera_pos, ortho_half, width, height);
Vec2 screen = world_to_screen(world_pos, camera_pos, ortho_half, width, height);
```
