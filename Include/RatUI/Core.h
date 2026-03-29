#pragma once

/** 
 * @file Core.h
 * @brief This file contains core type definitions and utilities for RatUI.
 */

#include "Core/Config.h"
#include "Core/Macro.h"
#include "Core/Types.h"
#include <limits>

namespace RatUI
{
    // === Type Traits ===

    template<typename>
    inline constexpr bool AlwaysFalse = false;

    /** Shorthand because I'm lazy and sick of typing std::numeric_limits<T>::max() all the time. */
    template<typename T>
    using Limits = std::numeric_limits<T>;

	template<typename T>
    inline constexpr bool HasFlag( T a_Value, T a_Flag )
    {
        return ( a_Value & a_Flag ) == a_Flag;
    }

} // namespace RatUI

#include "Core/Debug.inl"
#include "Core/Containers.inl"
#include "Core/Math.inl"
#include "Core/String.inl"
#include "Core/Memory.inl"
#include "Core/Units.inl"
#include "Core/Pool.inl"

#undef RATUI_TRY_EXPR
