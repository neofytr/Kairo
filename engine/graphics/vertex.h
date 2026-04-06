#pragma once

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/vec2.h"

namespace kairo {

// vertex layout for 2D quads — position, color, and tex coords
// keeping vec3 position so it extends naturally to 3D
struct Vertex {
    Vec3 position;
    Vec4 color;
    Vec2 tex_coords;
};

} // namespace kairo
