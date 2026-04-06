#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "physics/aabb.h"
#include <vector>

namespace kairo {

class Camera;
class Shader;

struct ShadowCaster {
    AABB bounds;  // the occluder
};

class ShadowSystem {
public:
    bool init();
    void shutdown();

    void clear_casters();
    void add_caster(const ShadowCaster& caster);

    // render shadows for a point light at `light_pos`
    // shadows are rendered as dark polygons covering occluded areas
    void render_shadows(const Vec2& light_pos, float light_radius,
                       const Camera& camera, float shadow_alpha = 0.5f);

private:
    Shader* m_shadow_shader = nullptr;
    u32 m_vao = 0;
    u32 m_vbo = 0;

    std::vector<ShadowCaster> m_casters;

    // compute shadow polygon vertices for one occluder
    void compute_shadow_geometry(const Vec2& light_pos, const AABB& occluder,
                                float light_radius, std::vector<float>& vertices);
};

} // namespace kairo
