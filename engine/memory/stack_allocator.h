#pragma once

#include "core/types.h"
#include <cstdlib>
#include <cassert>

namespace kairo {

// stack allocator — LIFO allocation pattern
// allocations must be freed in reverse order
// great for hierarchical/scoped allocations
class StackAllocator {
public:
    // marker for rolling back to a previous state
    using Marker = size_t;

    explicit StackAllocator(size_t size_bytes)
        : m_size(size_bytes), m_offset(0) {
        m_memory = static_cast<u8*>(std::malloc(size_bytes));
    }

    ~StackAllocator() {
        std::free(m_memory);
    }

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    // get a marker for the current position
    Marker get_marker() const { return m_offset; }

    // allocate with alignment
    void* allocate(size_t size, size_t alignment = 8) {
        size_t aligned = (m_offset + alignment - 1) & ~(alignment - 1);

        if (aligned + size > m_size) {
            assert(false && "stack allocator out of memory");
            return nullptr;
        }

        void* ptr = m_memory + aligned;
        m_offset = aligned + size;
        return ptr;
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // roll back to a marker (frees everything allocated after it)
    void free_to_marker(Marker marker) {
        assert(marker <= m_offset);
        m_offset = marker;
    }

    // free everything
    void reset() {
        m_offset = 0;
    }

    size_t used() const { return m_offset; }
    size_t capacity() const { return m_size; }

private:
    u8* m_memory = nullptr;
    size_t m_size = 0;
    size_t m_offset = 0;
};

} // namespace kairo
