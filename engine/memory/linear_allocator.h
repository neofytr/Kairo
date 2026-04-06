#pragma once

#include "core/types.h"
#include <cstdlib>
#include <cassert>

namespace kairo {

// linear (bump) allocator — extremely fast, no individual free
// allocations are sequential; reset() frees everything at once
// ideal for per-frame temporary data
class LinearAllocator {
public:
    explicit LinearAllocator(size_t size_bytes)
        : m_size(size_bytes), m_offset(0) {
        m_memory = static_cast<u8*>(std::malloc(size_bytes));
    }

    ~LinearAllocator() {
        std::free(m_memory);
    }

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    // allocate with alignment
    void* allocate(size_t size, size_t alignment = 8) {
        // align the current offset
        size_t aligned = (m_offset + alignment - 1) & ~(alignment - 1);

        if (aligned + size > m_size) {
            assert(false && "linear allocator out of memory");
            return nullptr;
        }

        void* ptr = m_memory + aligned;
        m_offset = aligned + size;
        m_allocation_count++;
        return ptr;
    }

    // allocate and construct a T
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // free everything at once
    void reset() {
        m_offset = 0;
        m_allocation_count = 0;
    }

    size_t used() const { return m_offset; }
    size_t capacity() const { return m_size; }
    size_t allocation_count() const { return m_allocation_count; }

private:
    u8* m_memory = nullptr;
    size_t m_size = 0;
    size_t m_offset = 0;
    size_t m_allocation_count = 0;
};

} // namespace kairo
