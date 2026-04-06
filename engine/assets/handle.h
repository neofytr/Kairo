#pragma once

#include "core/types.h"

namespace kairo {

// a type-safe handle to an asset
// internally just an index into the asset manager's storage
// the generation field detects use-after-unload
template<typename T>
struct Handle {
    u32 index = 0;
    u32 generation = 0;

    bool is_valid() const { return generation != 0; }
    bool operator==(const Handle& other) const { return index == other.index && generation == other.generation; }
    bool operator!=(const Handle& other) const { return !(*this == other); }
};

// null handle constant
template<typename T>
inline constexpr Handle<T> NULL_HANDLE = {};

} // namespace kairo
