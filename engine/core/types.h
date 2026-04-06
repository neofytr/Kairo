#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cassert>

namespace kairo {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

template<typename T>
using Unique = std::unique_ptr<T>;

template<typename T>
using Shared = std::shared_ptr<T>;

} // namespace kairo
