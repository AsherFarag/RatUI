#pragma once

/** 
 * @file Containers.inl
 * @brief This file contains container type definitions and utility functions for RatUI.
 * It is included by Core.h and should not be included directly by user code.
 * The container types and functions defined here are designed to be flexible and adaptable to different container implementations.
 */

#include "../Core.h"
#include <iterator>
#include <utility>

#ifndef RATUI_SPAN_IMPL
    #include <span>
    #define RATUI_SPAN_IMPL std::span
#endif // Default to std::span if no custom span implementation is provided.

#ifndef RATUI_ARRAY_IMPL
    #include <vector>
    #define RATUI_ARRAY_IMPL std::vector
#endif // Default to std::vector if no custom array implementation is provided.

#ifndef RATUI_FIXED_ARRAY_IMPL
    #include <array>
    #define RATUI_FIXED_ARRAY_IMPL std::array
#endif // Default to std::array if no custom fixed array implementation is provided.

#ifndef RATUI_SMALL_ARRAY_IMPL
    #include <array>
    #define RATUI_SMALL_ARRAY_IMPL RATUI_ARRAY_IMPL
#endif // TODO: Implement a SBO optimized small array and define RATUI_SMALL_ARRAY_IMPL to use it.

#ifndef RATUI_VARIANT_IMPL
    #include <variant>
    #define RATUI_VARIANT_IMPL std::variant
    #define RATUI_MONOSTATE_IMPL std::monostate
#endif // Default to std::variant if no custom variant implementation is provided.

#ifndef RATUI_OPTIONAL_IMPL
    #include <optional>
    #define RATUI_OPTIONAL_IMPL std::optional
    #define RATUI_NULLOPT_IMPL std::nullopt
#endif // Default to std::optional if no custom optional implementation is provided.

namespace RatUI
{
    // === Containers ===

    /** 
     * @brief Span is a non-owning view over a contiguous sequence of elements.
     * It is implemented using std::span by default, but can be customized by defining RATUI_SPAN_IMPL before including this header.
     * @tparam T The type of elements in the span.
     */
    template<typename T>
    using Span = RATUI_SPAN_IMPL<T>;

    /** 
     * @brief Array is a dynamically sized array container.
     * It is implemented using std::vector by default, but can be customized by defining RATUI_ARRAY_IMPL before including this header.
     * @tparam T The type of elements stored in the array.
     */ 
    template<typename T>
    using Array = RATUI_ARRAY_IMPL<T>;

    /** 
     * @brief FixedArray is a statically sized array container.
     * It is implemented using std::array by default, but can be customized by defining RATUI_FIXED_ARRAY_IMPL before including this header.
     * @tparam T The type of elements stored in the array.
     * @tparam N The number of elements in the array.
     */
    template<typename T, size N>
    using FixedArray = RATUI_FIXED_ARRAY_IMPL<T, N>;

    /**
     * @brief SmallArray is a container that optimizes for small sizes by using inline storage up to a certain capacity, and dynamically allocating memory for larger sizes.
     * TODO: Implement me!
     * @tparam T The type of elements stored in the array.
     * @tparam InlineSize The number of elements that can be stored inline before dynamic allocation is used.
     */
    template<typename T, size InlineSize>
    using SmallArray = RATUI_SMALL_ARRAY_IMPL<T /*, InlineSize */>;

    /**
     * @brief Variant is a type that can hold one of several alternative types, similar to std::variant.
     * @tparam Types The list of types that the variant can hold.
     */
    template<typename... Types>
    using Variant = RATUI_VARIANT_IMPL<Types...>;

    /**
     * @brief Monostate is a type that represents an empty state in a variant, similar to std::monostate.
     * It can be used as one of the alternative types in a Variant to represent a "no value" state.
     */
    using Monostate = RATUI_MONOSTATE_IMPL;

    /**
     * @brief Optional is a type that may or may not contain a value, similar to std::optional.
     * @tparam T The type of the value that may be contained in the optional.
     */
    template<typename T>
    using Optional = RATUI_OPTIONAL_IMPL<T>;
    inline constexpr auto NullOpt = RATUI_NULLOPT_IMPL;

    // === Container Access ===

    template<typename Container>
    constexpr decltype(auto) Begin( const Container& a_Container )
    {
        return std::begin( a_Container );
    }

    template<typename Container>
    constexpr decltype(auto) End( const Container& a_Container )
    {
        return std::end( a_Container );
    }

    template<typename Container>
    constexpr decltype(auto) Size( const Container& a_Container )
    {
        return std::size( a_Container );
    }

    template<typename Container>
    constexpr decltype(auto) Empty( const Container& a_Container )
    {
        return std::empty( a_Container );
    }

    // === Container Modification ===

    template<typename Container, typename... Args>
    constexpr decltype(auto) PushBack( Container& a_Container, Args&&... a_Args )
    {
             RATUI_TRY_EXPR( a_Container.push_back( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.PushBack( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.pushBack( std::forward<Args>( a_Args )... ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support PushBack, push_back, or pushBack." ); }
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) EmplaceBack( Container& a_Container, Args&&... a_Args )
    {
             RATUI_TRY_EXPR( a_Container.emplace_back( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.EmplaceBack( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.emplaceBack( std::forward<Args>( a_Args )... ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support EmplaceBack, emplace_back, or emplaceBack." ); }
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Insert( Container& a_Container, Args&&... a_Args )
    {
             RATUI_TRY_EXPR( a_Container.insert( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.Insert( std::forward<Args>( a_Args )... ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Insert or insert." ); }
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Emplace( Container& a_Container, Args&&... a_Args )
    {
             RATUI_TRY_EXPR( a_Container.emplace( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.Emplace( std::forward<Args>( a_Args )... ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Emplace or emplace." ); }
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Erase( Container& a_Container, Args&&... a_Args )
    {
             RATUI_TRY_EXPR( a_Container.erase( std::forward<Args>( a_Args )... ) )
        else RATUI_TRY_EXPR( a_Container.Erase( std::forward<Args>( a_Args )... ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Erase or erase." ); }
    }

    template<typename Container>
    constexpr void PopBack( Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.pop_back() )
        else RATUI_TRY_EXPR( a_Container.PopBack() )
        else RATUI_TRY_EXPR( a_Container.popBack() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support PopBack, pop_back, or popBack." ); }
    }

    template<typename Container>
    constexpr void Clear( Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.clear() )
        else RATUI_TRY_EXPR( a_Container.Clear() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Clear or clear." ); }
    }

    template<typename Container>
    constexpr void Reserve( Container& a_Container, const size a_Capacity )
    {
             RATUI_TRY_EXPR( a_Container.reserve( a_Capacity ) )
        else RATUI_TRY_EXPR( a_Container.Reserve( a_Capacity ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Reserve or reserve." ); }
    }

    template<typename Container>
    constexpr void Resize( Container& a_Container, const size a_Size )
    {
             RATUI_TRY_EXPR( a_Container.resize( a_Size ) )
        else RATUI_TRY_EXPR( a_Container.Resize( a_Size ) )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Resize or resize." ); }
    }

    // === Element Access ===

    template<typename Container>
    constexpr decltype(auto) Front( Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.front() )
        else RATUI_TRY_EXPR( a_Container.Front() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Front or front." ); }
    }

    template<typename Container>
    constexpr decltype(auto) Front( const Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.front() )
        else RATUI_TRY_EXPR( a_Container.Front() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Front or front." ); }
    }

    template<typename Container>
    constexpr decltype(auto) Back( Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.back() )
        else RATUI_TRY_EXPR( a_Container.Back() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Back or back." ); }
    }

    template<typename Container>
    constexpr decltype(auto) Back( const Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.back() )
        else RATUI_TRY_EXPR( a_Container.Back() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Back or back." ); }
    }

    template<typename Container>
    constexpr decltype(auto) At( Container& a_Container, const size a_Index )
    {
             RATUI_TRY_EXPR( a_Container.at( a_Index ) )
        else RATUI_TRY_EXPR( a_Container.At( a_Index ) )
        else RATUI_TRY_EXPR( a_Container[ a_Index ] )
        else { static_assert( AlwaysFalse<Container>, "Container does not support At, at, or operator[]." ); }
    }

    template<typename Container>
    constexpr decltype(auto) At( const Container& a_Container, const size a_Index )
    {
             RATUI_TRY_EXPR( a_Container.at( a_Index ) )
        else RATUI_TRY_EXPR( a_Container.At( a_Index ) )
        else RATUI_TRY_EXPR( a_Container[ a_Index ] )
        else { static_assert( AlwaysFalse<Container>, "Container does not support At, at, or operator[]." ); }
    }

    template<typename Container>
    constexpr decltype(auto) Data( Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.data() )
        else RATUI_TRY_EXPR( a_Container.Data() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Data or data." ); }
    }

    template<typename Container>
    constexpr decltype(auto) Data( const Container& a_Container )
    {
             RATUI_TRY_EXPR( a_Container.data() )
        else RATUI_TRY_EXPR( a_Container.Data() )
        else { static_assert( AlwaysFalse<Container>, "Container does not support Data or data." ); }
    }

} // namespace RatUI
