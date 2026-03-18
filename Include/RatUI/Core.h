#pragma once

/** 
 * @file Core.h
 * @brief This file contains core type definitions and utilities for RatUI.
 */

#include "Config.h"
#include <cstdint>

/** @brief Helper macro to conditionally compile an expression if it is valid. */
#define RATUI_TRY_EXPR( _Expr ) \
    if constexpr ( requires { _Expr; } ) \
    { \
        _Expr; \
    }

namespace RatUI
{
    // === Primitives ===

    using f32 = float;
    using f64 = double;
    
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using size = std::size_t;

    // === Type Traits ===

    template<typename>
    inline constexpr bool AlwaysFalse = false;

} // namespace RatUI

#include "Containers.inl"
#include "Math.inl"

#undef RATUI_TRY_EXPR