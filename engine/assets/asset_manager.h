#pragma once

#include "core/types.h"
#include "core/log.h"
#include "assets/handle.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <functional>

namespace kairo {

// generic asset manager that can store any asset type
// assets are loaded by path, cached, and accessed via handles
// supports custom loader functions per asset type
template<typename T>
class AssetManager {
public:
    using LoaderFunc = std::function<std::optional<T>(const std::string&)>;

    // set the function used to load assets of this type
    void set_loader(LoaderFunc loader) {
        m_loader = std::move(loader);
    }

    // load an asset from path (or return existing handle if already loaded)
    Handle<T> load(const std::string& path) {
        // check if already loaded
        auto it = m_path_map.find(path);
        if (it != m_path_map.end()) {
            auto& slot = m_slots[it->second];
            if (slot.loaded) {
                slot.ref_count++;
                return { it->second, slot.generation };
            }
        }

        if (!m_loader) {
            log::error("asset manager: no loader set for type");
            return {};
        }

        auto result = m_loader(path);
        if (!result.has_value()) {
            log::error("asset manager: failed to load '%s'", path.c_str());
            return {};
        }

        u32 index;
        if (!m_free_indices.empty()) {
            index = m_free_indices.back();
            m_free_indices.pop_back();
        } else {
            index = static_cast<u32>(m_slots.size());
            m_slots.push_back({});
        }

        auto& slot = m_slots[index];
        slot.asset = std::move(result.value());
        slot.path = path;
        slot.loaded = true;
        slot.generation++;
        slot.ref_count = 1;

        m_path_map[path] = index;

        return { index, slot.generation };
    }

    // get a reference to the asset (asserts handle is valid)
    T& get(Handle<T> handle) {
        assert(is_valid(handle));
        return m_slots[handle.index].asset;
    }

    const T& get(Handle<T> handle) const {
        assert(is_valid(handle));
        return m_slots[handle.index].asset;
    }

    // check if handle is still valid
    bool is_valid(Handle<T> handle) const {
        if (handle.index >= m_slots.size()) return false;
        auto& slot = m_slots[handle.index];
        return slot.loaded && slot.generation == handle.generation;
    }

    // release a handle (decrement ref count, unload if zero)
    void release(Handle<T> handle) {
        if (!is_valid(handle)) return;

        auto& slot = m_slots[handle.index];
        slot.ref_count--;

        if (slot.ref_count <= 0) {
            m_path_map.erase(slot.path);
            slot.loaded = false;
            slot.asset = T{};
            slot.path.clear();
            m_free_indices.push_back(handle.index);
        }
    }

    // unload everything
    void clear() {
        m_slots.clear();
        m_path_map.clear();
        m_free_indices.clear();
    }

    size_t loaded_count() const {
        size_t count = 0;
        for (auto& s : m_slots) {
            if (s.loaded) count++;
        }
        return count;
    }

private:
    struct Slot {
        T asset{};
        std::string path;
        u32 generation = 0;
        i32 ref_count = 0;
        bool loaded = false;
    };

    std::vector<Slot> m_slots;
    std::vector<u32> m_free_indices;
    std::unordered_map<std::string, u32> m_path_map;
    LoaderFunc m_loader;
};

} // namespace kairo
