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

    /**
     * @brief This is the main template for making RatUI use your custom types instead of the built-in ones. 
     * Specialize RatUI::CoreTraits<T> for your type to use it with the generic functions provided by RatUI.
     */
    template<typename _Container>
    struct CoreTraits
    {
        static_assert(AlwaysFalse<_Container>,
            "No CoreTraits specialization found for this type. "
            "Please either remove RATUI_OVERRIDE_*_IMPL to use the default implementation based on standard library types, "
            "or provide a specialization of CoreTraits for your custom type.");
    };

} // namespace RatUI

#include "Core/Debug.inl"
#include "Core/Containers.inl"
#include "Core/Math.inl"
#include "Core/String.inl"
#include "Core/Memory.inl"
#include "Core/Units.inl"
#include "Core/Pool.inl"

#undef RATUI_TRY_EXPR
