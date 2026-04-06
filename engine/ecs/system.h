#pragma once

namespace kairo {

class World;

// base class for ECS systems
// systems contain logic that operates on entities with specific components
// keep systems stateless where possible — data lives in components
class System {
public:
    virtual ~System() = default;

    virtual void init(World& world) {}
    virtual void update(World& world, float dt) {}
    virtual void render(World& world) {}
};

} // namespace kairo
