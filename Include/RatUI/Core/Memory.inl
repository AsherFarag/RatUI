#pragma once

/**
 * @file Memory.inl
 * @brief This file contains memory management utilities for RatUI, including smart pointer definitions and helper functions.
 * It is included by Core.h and should not be included directly by user code.
 */

#include "../Core.h"

#ifndef RATUI_UNIQUE_PTR_IMPL
    #include <memory>
    #define RATUI_UNIQUE_PTR_IMPL std::unique_ptr

    namespace RatUI
    {
        template<typename T, typename... Args>
        constexpr auto MakeUnique( Args&&... a_Args )
        {
            return std::make_unique<T>( std::forward<Args>( a_Args )... );
        }
    }

#endif // Default to std::unique_ptr if no custom unique pointer implementation is provided.

#ifndef RATUI_SHARED_PTR_IMPL
    #include <memory>
    #define RATUI_SHARED_PTR_IMPL std::shared_ptr

    namespace RatUI
    {
        template<typename T, typename... Args>
        constexpr auto MakeShared( Args&&... a_Args )
        {
            return std::make_shared<T>( std::forward<Args>( a_Args )... );
        }
    }

#endif // Default to std::shared_ptr if no custom shared pointer implementation is provided.

namespace RatUI
{
    /** 
     * @brief Unique is a smart pointer that owns and manages another object through a pointer and disposes of that object when the Unique goes out of scope.
     * It is implemented using std::unique_ptr by default, but can be customized by defining RATUI_UNIQUE_PTR_IMPL before including this header.
     * @tparam T The type of the object being managed.
     */
    template<typename T>
    using Unique = RATUI_UNIQUE_PTR_IMPL<T>;

    /** 
     * @brief Shared is a smart pointer that retains shared ownership of an object through a pointer. 
     * Multiple Shared instances can own the same object, and the object is destroyed when the last Shared owning it is destroyed or reset.
     * It is implemented using std::shared_ptr by default, but can be customized by defining RATUI_SHARED_PTR_IMPL before including this header.
     * @tparam T The type of the object being managed.
     */
    template<typename T>
    using Shared = RATUI_SHARED_PTR_IMPL<T>;

} // namespace RatUI