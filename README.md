# Kairo

A 2D game engine written from scratch in C++17. No Unity, no Unreal, no off-the-shelf ECS library. Just OpenGL 4.6, a lot of template metaprogramming, and questionable life choices.

I built this as a learning project and portfolio piece -- the goal was to understand how each major engine system works by implementing it myself. The result is a ~11k line engine with an archetype ECS, a batch renderer, custom physics, Lua scripting, and enough tooling (editor, profiler, debug draw) to actually make a small game with it.

<!-- screenshot here -->

## The ECS

The ECS was the hardest part to get right and the most satisfying once it clicked. It's archetype-based, similar in spirit to flecs or Unity DOTS, but written from scratch.

Entities are 64-bit handles: 32 bits index, 32 bits generation. The generation counter catches use-after-destroy bugs at runtime instead of letting you silently corrupt data through a stale handle.

```cpp
struct Entity {
    u64 id = 0;
    u32 index() const { return static_cast<u32>(id & 0xFFFFFFFF); }
    u32 generation() const { return static_cast<u32>(id >> 32); }
};
```

Components are stored in packed arrays grouped by archetype (the unique set of component types an entity has). When you add or remove a component, the entity moves to a different archetype. This gives you cache-friendly iteration -- a query over `<Transform, Sprite>` touches only contiguous memory, no indirection.

The query API is compile-time:

```cpp
world.query<Transform, Sprite>([](Entity e, Transform& t, Sprite& s) {
    t.position += s.velocity * dt;
});
```

No runtime type lookups during iteration. The template instantiation resolves which archetype tables to visit and which column offsets to use.

## Renderer

OpenGL 4.6 batch renderer. Sprites are sorted by layer and z-depth (deferred sort -- collect everything, sort once, draw), then batched with up to 16 textures per draw call using texture arrays. This means the common case of "a bunch of sprites with different textures" doesn't turn into one draw call per sprite.

On top of the sprite renderer:
- 2D point lights with quadratic distance falloff, rendered to a light map and composited with multiplicative blending
- 2D shadow casting from point lights (raycasting against geometry to build shadow volumes)
- Post-processing chain: framebuffer ping-pong for bloom and vignette
- A frame graph system for declaring render passes and their dependencies in a data-driven way

## Physics

Custom 2D physics. Not trying to compete with Box2D -- this is purpose-built for the kind of top-down game the demo implements.

- AABB and circle colliders, plus AABB-circle pairs
- Impulse-based collision resolution with restitution and friction
- Spatial grid broadphase (cells are tuned to average collider size)
- Collision layers and masks
- Raycasting against the spatial grid
- Distance and revolute joints

The spatial grid was a clear win over brute-force. For the demo's ~200 entities, broadphase cut collision pair checks by roughly 90%.

## Scripting

Lua 5.4 is embedded for gameplay logic. The engine exposes entity manipulation, input, physics queries, and audio to Lua. Scripts can be hot-reloaded at runtime -- edit a `.lua` file, the engine detects the change and reloads it without restarting. Essential for iteration speed.

## Editor

ImGui-based editor that runs in-engine:
- Entity hierarchy and component inspector
- Transform gizmos (translate, rotate, scale)
- Lua console for live commands
- Profiler overlay (frame times, system timings)
- Asset browser
- Undo/redo stack

It's not a shipping-quality editor, but it's good enough to place entities, tweak values, and debug problems without recompiling.

## Other Systems Worth Mentioning

**Audio.** miniaudio backend. Spatial audio with distance-based falloff. Fire-and-forget API for sound effects, managed playback for music.

**Particles.** Data-oriented particle system. Emitters are configured with min/max ranges for lifetime, velocity, color, size. Update is a flat loop over a packed array -- no per-particle allocations.

**Memory.** Three custom allocators: pool (fixed-size blocks, O(1) alloc/free), linear (bump pointer, bulk reset), and stack (LIFO, scoped scratch memory). Used in the particle system and physics broadphase where allocation patterns are predictable.

**Math.** Custom vec2/3/4, mat4. SSE-optimized SIMD variants for the hot paths (matrix multiply, batch transforms). No GLM dependency.

**Gameplay.** Tilemap loading, A* pathfinding, AI steering behaviors (seek, flee, arrive, wander), animation state machine. These are more "game library" than "engine" but they were useful for the demo.

**Networking.** Basic UDP with entity state synchronization. Enough to prototype multiplayer, not enough to ship it.

**Job System.** Thread pool with a `parallel_for` utility. Used to parallelize particle updates and physics broadphase queries.

## Architecture

The engine is split into 16 modules with minimal cross-dependencies:

```
engine/
  core/         types, platform, logging, time
  math/         vectors, matrices, SIMD
  ecs/          world, archetypes, entities, queries
  graphics/     renderer, shaders, textures, lights, shadows, post-processing
  input/        keyboard, mouse, gamepad
  scene/        scene stack (push/pop/switch), JSON serialization
  assets/       asset loading and caching
  physics/      colliders, rigidbodies, spatial grid, raycasting, joints
  particles/    emitters, particle pools
  events/       event bus, typed events
  gameplay/     tilemap, pathfinding, steering, animation
  editor/       ImGui panels, gizmos, undo system
  scripting/    Lua bindings, hot-reload
  audio/        miniaudio wrapper, spatial audio
  memory/       pool, linear, stack allocators
```

Key design decisions:

**Why archetype ECS over sparse sets?** Iteration performance. The engine's workload is "iterate over all entities with components X and Y" far more often than "add/remove components on a single entity." Archetypes make the common case fast at the cost of slower structural changes. For a game with mostly static component sets, that's the right trade.

**Why custom physics?** Box2D is great, but it's a general-purpose 2D physics engine. I wanted something tighter: no continuous collision detection I don't need, collision layers that map directly to game concepts, and a broadphase tuned for the entity counts I'm actually dealing with. Also, I wanted to understand how impulse resolution actually works.

**Why OpenGL 4.6?** Vulkan would be more impressive on a resume but would have tripled the renderer development time for no visual difference in a 2D engine. OpenGL 4.6 gives me DSA (direct state access), which cleans up the API considerably, and the batch renderer keeps draw calls low enough that driver overhead isn't a real problem.

## Demo

The repo includes a top-down arena shooter (`sandbox/`) that exercises most of the engine: ECS-driven gameplay, physics collisions, particle effects, point lighting with shadows, Lua-scripted enemy AI, and spatial audio. It's not a polished game, but it proves the engine works end-to-end.

## Building

Requirements:
- C++17 compiler (GCC 9+ or Clang 10+)
- CMake 3.20+
- GLFW 3.3+ (install via your package manager)

Everything else is bundled in `external/`: GLAD, stb_image, ImGui, Lua 5.4, miniaudio, nlohmann/json.

```bash
# install GLFW (Ubuntu/Debian)
sudo apt install libglfw3-dev

# build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# run the demo
./build/bin/sandbox

# run tests (263 of them)
./build/bin/tests
```

Developed on Linux. Should be reasonably portable to other platforms with OpenGL 4.6 support, but I haven't tested Windows or macOS builds.

## Tests

263 automated tests covering the ECS, math library, physics, memory allocators, event system, and gameplay utilities. Not exhaustive, but enough to catch regressions in the core systems.

```bash
./build/bin/tests
```
