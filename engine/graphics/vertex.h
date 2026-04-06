#pragma once

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/vec2.h"

namespace kairo {

struct Vertex {
    Vec3 position;
    Vec4 color;
    Vec2 tex_coords;
    float tex_index = 0.0f; // which texture slot this vertex uses
};

} // namespace kairo
