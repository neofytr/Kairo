#pragma once

#include "core/types.h"
#include "graphics/vertex.h"
#include "math/mat4.h"

namespace kairo {

// low-level batch renderer
// accumulates vertices and flushes to GPU in batches
class BatchRenderer {
public:
    static constexpr u32 MAX_QUADS    = 10000;
    static constexpr u32 MAX_VERTICES = MAX_QUADS * 4;
    static constexpr u32 MAX_INDICES  = MAX_QUADS * 6;

    bool init();
    void shutdown();

    void begin_batch();
    void end_batch();
    void flush();

    // submit a quad as 4 vertices (caller transforms them)
    void submit_quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3);

    u32 get_draw_call_count() const { return m_draw_calls; }
    u32 get_quad_count() const { return m_quad_count; }
    void reset_stats() { m_draw_calls = 0; m_quad_count = 0; }

private:
    u32 m_vao = 0;
    u32 m_vbo = 0;
    u32 m_ibo = 0;

    Vertex* m_buffer_base = nullptr;  // start of CPU-side vertex buffer
    Vertex* m_buffer_ptr  = nullptr;  // current write position
    u32 m_index_count = 0;

    u32 m_draw_calls = 0;
    u32 m_quad_count = 0;
};

} // namespace kairo
