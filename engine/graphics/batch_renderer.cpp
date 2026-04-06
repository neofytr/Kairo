#include "graphics/batch_renderer.h"
#include "graphics/texture.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

bool BatchRenderer::init() {
    m_buffer_base = new Vertex[MAX_VERTICES];

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position));

    // color (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, color));

    // tex coords (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, tex_coords));

    // tex index (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, tex_index));

    // index buffer
    u32* indices = new u32[MAX_INDICES];
    u32 offset = 0;
    for (u32 i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(u32), indices, GL_STATIC_DRAW);

    delete[] indices;
    glBindVertexArray(0);

    log::info("batch renderer initialized (max %d quads, %d texture slots)", MAX_QUADS, MAX_TEXTURE_SLOTS);
    return true;
}

void BatchRenderer::shutdown() {
    delete[] m_buffer_base;
    m_buffer_base = nullptr;

    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
}

void BatchRenderer::begin_batch() {
    m_buffer_ptr = m_buffer_base;
    m_index_count = 0;
    m_texture_slot_count = 1; // reset, keep slot 0 (white)
}

void BatchRenderer::end_batch() {
    flush();
}

void BatchRenderer::flush() {
    if (m_index_count == 0) return;

    // bind all active textures
    for (u32 i = 0; i < m_texture_slot_count; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_texture_slots[i]);
    }

    // upload and draw
    u32 data_size = static_cast<u32>((m_buffer_ptr - m_buffer_base) * sizeof(Vertex));
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, m_buffer_base);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, nullptr);

    m_draw_calls++;

    m_buffer_ptr = m_buffer_base;
    m_index_count = 0;
    m_texture_slot_count = 1;
}

void BatchRenderer::submit_quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    if (m_index_count >= MAX_INDICES) {
        flush();
    }

    *m_buffer_ptr++ = v0;
    *m_buffer_ptr++ = v1;
    *m_buffer_ptr++ = v2;
    *m_buffer_ptr++ = v3;

    m_index_count += 6;
    m_quad_count++;
}

void BatchRenderer::submit_textured_quad(Vertex v0, Vertex v1, Vertex v2, Vertex v3, const Texture* texture) {
    if (!texture) {
        // no texture — use white (slot 0), which makes color pass through
        submit_quad(v0, v1, v2, v3);
        return;
    }

    i32 slot = find_or_add_texture(texture->get_id());
    if (slot < 0) {
        // texture slots full — flush and retry
        flush();
        slot = find_or_add_texture(texture->get_id());
    }

    float tex_idx = static_cast<float>(slot);
    v0.tex_index = tex_idx;
    v1.tex_index = tex_idx;
    v2.tex_index = tex_idx;
    v3.tex_index = tex_idx;

    submit_quad(v0, v1, v2, v3);
}

i32 BatchRenderer::find_or_add_texture(u32 texture_id) {
    // check if already bound
    for (u32 i = 0; i < m_texture_slot_count; i++) {
        if (m_texture_slots[i] == texture_id) {
            return static_cast<i32>(i);
        }
    }

    if (m_texture_slot_count >= MAX_TEXTURE_SLOTS) {
        return -1; // full
    }

    m_texture_slots[m_texture_slot_count] = texture_id;
    return static_cast<i32>(m_texture_slot_count++);
}

} // namespace kairo
