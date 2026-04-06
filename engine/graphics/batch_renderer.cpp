#include "graphics/batch_renderer.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

bool BatchRenderer::init() {
    // allocate CPU-side vertex buffer
    m_buffer_base = new Vertex[MAX_VERTICES];

    // create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // create VBO — dynamic, we update it every frame
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // vertex layout: position (3f), color (4f), tex_coords (2f)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, color));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, tex_coords));

    // generate index buffer — quad indices follow a fixed pattern
    // 0,1,2, 2,3,0 for each quad
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

    log::info("batch renderer initialized (max %d quads per batch)", MAX_QUADS);
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
}

void BatchRenderer::end_batch() {
    flush();
}

void BatchRenderer::flush() {
    if (m_index_count == 0) return;

    // upload vertex data to GPU
    u32 data_size = static_cast<u32>((m_buffer_ptr - m_buffer_base) * sizeof(Vertex));
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, m_buffer_base);

    // draw
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, nullptr);

    m_draw_calls++;

    // reset for next batch
    m_buffer_ptr = m_buffer_base;
    m_index_count = 0;
}

void BatchRenderer::submit_quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    // if batch is full, flush and start a new one
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

} // namespace kairo
