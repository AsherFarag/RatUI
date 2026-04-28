#pragma once
#include <cstddef>
#include <cstdint>

namespace RatUI
{
    using f32 = float;
    using f64 = double;

    using c8  = char;
    using c16 = char16_t;
    using c32 = char32_t;

    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using size = std::size_t;
    using iptr = std::intptr_t;
    using uptr = std::uintptr_t;

    /** 
     * @brief Type for representing a Unicode code point. 
     * This is a 32-bit type that can represent any Unicode code point, 
     * including those outside the Basic Multilingual Plane (BMP). 
     */
    using codepoint = c32;

} // namespace RatUI