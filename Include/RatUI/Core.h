#pragma once

/** 
 * @file Core.h
 * @brief This file contains core type definitions and utilities for RatUI.
 */

#include "Core/Config.h"
#include <cstddef>
#include <cstdint>
#include <limits>

/** @brief Helper macro to conditionally compile and return an expression if it is valid. */
#define RATUI_TRY_EXPR( _Expr ) \
    if constexpr ( requires { _Expr; } ) \
    { \
        return _Expr; \
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

    /** Shorthand because I'm lazy and sick of typing std::numeric_limits<T>::max() all the time. */
    template<typename T>
    using Limits = std::numeric_limits<T>;

    // === Config Definitions ===

    inline constexpr size c_ChildrenSBOSize = RATUI_CONFIG_CHILDREN_SBO_SIZE;

} // namespace RatUI

#include "Core/Containers.inl"
#include "Core/Math.inl"
#include "Core/String.inl"
#include "Core/Memory.inl"
#include "Core/Units.inl"

#undef RATUI_TRY_EXPR
