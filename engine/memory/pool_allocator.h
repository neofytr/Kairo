#pragma once

#include "core/types.h"
#include <vector>
#include <memory>
#include <cassert>

namespace kairo {

// fixed-size pool allocator — O(1) alloc/free, zero fragmentation
// uses chunked storage so pointers remain stable when the pool grows
template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(size_t chunk_size = 1024)
        : m_chunk_size(chunk_size) {
        grow();
    }

    template<typename... Args>
    T* allocate(Args&&... args) {
        if (m_free_list.empty()) {
            grow();
        }

        size_t index = m_free_list.back();
        m_free_list.pop_back();
        m_alive_count++;

        T* ptr = get_ptr(index);
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    void deallocate(T* ptr) {
        // find which chunk this pointer belongs to
        size_t index = ptr_to_index(ptr);
        assert(index < m_capacity);

        ptr->~T();
        m_free_list.push_back(index);
        m_alive_count--;
    }

    size_t capacity() const { return m_capacity; }
    size_t alive_count() const { return m_alive_count; }

private:
    size_t m_chunk_size;
    size_t m_capacity = 0;
    size_t m_alive_count = 0;

    // chunks of raw storage — pointers into these are stable
    struct Chunk {
        std::unique_ptr<u8[]> data;
        size_t count;
    };
    std::vector<Chunk> m_chunks;
    std::vector<size_t> m_free_list;

    void grow() {
        size_t base = m_capacity;
        auto chunk_data = std::make_unique<u8[]>(m_chunk_size * sizeof(T));
        m_chunks.push_back({ std::move(chunk_data), m_chunk_size });
        m_capacity += m_chunk_size;

        // add new indices to free list (in reverse for LIFO order)
        for (size_t i = base + m_chunk_size; i > base; i--) {
            m_free_list.push_back(i - 1);
        }
    }

    T* get_ptr(size_t index) {
        size_t chunk_idx = index / m_chunk_size;
        size_t local_idx = index % m_chunk_size;
        return reinterpret_cast<T*>(m_chunks[chunk_idx].data.get()) + local_idx;
    }

    size_t ptr_to_index(T* ptr) {
        for (size_t c = 0; c < m_chunks.size(); c++) {
            T* base = reinterpret_cast<T*>(m_chunks[c].data.get());
            if (ptr >= base && ptr < base + m_chunks[c].count) {
                return c * m_chunk_size + static_cast<size_t>(ptr - base);
            }
        }
        assert(false && "pointer not from this pool");
        return 0;
    }
};

} // namespace kairo
