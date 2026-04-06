#pragma once

#include "core/types.h"
#include "graphics/vertex.h"
#include "math/mat4.h"

namespace kairo {

class Texture;

class BatchRenderer {
public:
    static constexpr u32 MAX_QUADS       = 10000;
    static constexpr u32 MAX_VERTICES    = MAX_QUADS * 4;
    static constexpr u32 MAX_INDICES     = MAX_QUADS * 6;
    static constexpr u32 MAX_TEXTURE_SLOTS = 16; // match the shader

    bool init();
    void shutdown();

    void begin_batch();
    void end_batch();
    void flush();

    void submit_quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3);

    // submit a textured quad — binds the texture to a slot and sets tex_index on vertices
    void submit_textured_quad(Vertex v0, Vertex v1, Vertex v2, Vertex v3, const Texture* texture);

    u32 get_draw_call_count() const { return m_draw_calls; }
    u32 get_quad_count() const { return m_quad_count; }
    void reset_stats() { m_draw_calls = 0; m_quad_count = 0; }

    // set the default white texture (created by the renderer)
    void set_white_texture(u32 texture_id) { m_texture_slots[0] = texture_id; }

private:
    u32 m_vao = 0;
    u32 m_vbo = 0;
    u32 m_ibo = 0;

    Vertex* m_buffer_base = nullptr;
    Vertex* m_buffer_ptr  = nullptr;
    u32 m_index_count = 0;

    // texture batching — slot 0 is always the white texture
    u32 m_texture_slots[MAX_TEXTURE_SLOTS] = {};
    u32 m_texture_slot_count = 1; // 0 is white

    u32 m_draw_calls = 0;
    u32 m_quad_count = 0;

    // find or assign a texture slot, returns the slot index
    // returns -1 if slots are full (need to flush)
    i32 find_or_add_texture(u32 texture_id);
};

} // namespace kairo
