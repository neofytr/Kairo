#pragma once

#include "core/types.h"
#include <vector>
#include <cassert>

namespace kairo {

// fixed-size pool allocator — O(1) alloc/free, zero fragmentation
// great for components, particles, or any fixed-size objects
template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(size_t capacity = 1024) {
        m_pool.resize(capacity);
        m_free_list.reserve(capacity);
        // build free list in reverse so we allocate from the front
        for (size_t i = capacity; i > 0; i--) {
            m_free_list.push_back(i - 1);
        }
    }

    // allocate a slot and construct an object in it
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (m_free_list.empty()) {
            // grow the pool
            size_t old_size = m_pool.size();
            size_t new_size = old_size * 2;
            m_pool.resize(new_size);
            for (size_t i = new_size; i > old_size; i--) {
                m_free_list.push_back(i - 1);
            }
        }

        size_t index = m_free_list.back();
        m_free_list.pop_back();
        m_alive_count++;

        // placement new
        T* ptr = &m_pool[index];
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    // free a slot (call destructor, return to pool)
    void deallocate(T* ptr) {
        assert(ptr >= &m_pool[0] && ptr < &m_pool[0] + m_pool.size());

        ptr->~T();
        size_t index = static_cast<size_t>(ptr - &m_pool[0]);
        m_free_list.push_back(index);
        m_alive_count--;
    }

    size_t capacity() const { return m_pool.size(); }
    size_t alive_count() const { return m_alive_count; }

private:
    std::vector<T> m_pool;
    std::vector<size_t> m_free_list;
    size_t m_alive_count = 0;
};

} // namespace kairo
