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

#ifndef RATUI_OVERRIDE_SPAN_IMPL
    #include <span>

    namespace RatUI
    {
        template<typename T>
        using SpanImpl = std::span<T>;

        template<typename _ElementType>
        struct CoreTraits<SpanImpl<_ElementType>>
        {
            using Type        = SpanImpl<_ElementType>;
            using ElementType = typename Type::element_type;
            using SizeType    = typename Type::size_type;
            using Iter        = typename Type::iterator;
            using RIter       = typename Type::reverse_iterator;

        // C++23 added const reverse iterators to std::span
        #if __cplusplus >= 202300L
            using ConstIter   = typename Type::const_iterator;
            using ConstRIter  = typename Type::const_reverse_iterator;
        #endif
        
            static constexpr SizeType DynamicExtent = std::dynamic_extent;

            // Capacity

            static constexpr SizeType Size(const Type& a_Container)
            {
                return a_Container.size();
            }

            static constexpr bool Empty(const Type& a_Container)
            {
                return a_Container.empty();
            }

            // Element access

            static constexpr ElementType& RawAt(Type& a_Container, SizeType a_Index)
            {
                return a_Container[a_Index];
            }

            static constexpr const ElementType& RawAt(const Type& a_Container, SizeType a_Index)
            {
                return a_Container[a_Index];
            }

            static constexpr ElementType& Front(Type& a_Container)
            {
                return a_Container.front();
            }

            static constexpr const ElementType& Front(const Type& a_Container)
            {
                return a_Container.front();
            }

            static constexpr ElementType& Back(Type& a_Container)
            {
                return a_Container.back();
            }

            static constexpr const ElementType& Back(const Type& a_Container)
            {
                return a_Container.back();
            }

            static constexpr ElementType* Data(Type& a_Container)
            {
                return a_Container.data();
            }

            static constexpr const ElementType* Data(const Type& a_Container)
            {
                return a_Container.data();
            }

            // Iterators

            static constexpr Iter Begin(Type& a_Container) { return a_Container.begin(); }
            static constexpr Iter End(Type& a_Container) { return a_Container.end(); }

            // Reverse Iterators

            static constexpr RIter RBegin(Type& a_Container) { return a_Container.rbegin(); }
            static constexpr RIter REnd(Type& a_Container) { return a_Container.rend(); }

		#if __cplusplus >= 202300L
			static constexpr ConstIter Begin( const Type& a_Container ) { return a_Container.begin(); }
			static constexpr ConstIter End( const Type& a_Container ) { return a_Container.end(); }
			static constexpr ConstRIter RBegin( const Type& a_Container ) { return a_Container.rbegin(); }
			static constexpr ConstRIter REnd( const Type& a_Container ) { return a_Container.rend(); }
        #endif 
        };

    } // namespace RatUI

#endif // Default to std::span if no custom span implementation is provided.

#ifndef RATUI_OVERRIDE_ARRAY_IMPL
    #include <vector>

    namespace RatUI
    {
        template<typename T>
        using ArrayImpl = std::vector<T>;

        template<typename _ElementType>
        struct CoreTraits<ArrayImpl<_ElementType>>
        {
            using Type        = ArrayImpl<_ElementType>;
            using ElementType = typename Type::value_type;
            using SizeType    = typename Type::size_type;
            using Iter        = typename Type::iterator;
            using ConstIter   = typename Type::const_iterator;
            using RIter       = typename Type::reverse_iterator;
            using ConstRIter  = typename Type::const_reverse_iterator;
        
            // Capacity
        
            static constexpr SizeType Size(const Type& a_Container)
            {
                return a_Container.size();
            }
        
            static constexpr bool Empty(const Type& a_Container)
            {
                return a_Container.empty();
            }
        
            static constexpr void Reserve(Type& a_Container, SizeType a_Count)
            {
                a_Container.reserve(a_Count);
            }
        
            static constexpr void Resize(Type& a_Container, SizeType a_Count)
            {
                a_Container.resize(a_Count);
            }
        
            static constexpr void Clear(Type& a_Container)
            {
                a_Container.clear();
            }
        
            // Element access
        
            static constexpr ElementType& RawAt(Type& a_Container, SizeType a_Index)
            {
                return a_Container[a_Index];
            }
        
            static constexpr const ElementType& RawAt(const Type& a_Container, SizeType a_Index) 
            {
                return a_Container[a_Index];
            }

            static constexpr ElementType& At(Type& a_Container, SizeType a_Index)
            {
                return a_Container.at(a_Index);
            }

            static constexpr const ElementType& At(const Type& a_Container, SizeType a_Index)
            {
                return a_Container.at(a_Index);
            }
        
            static constexpr ElementType& Front(Type& a_Container)
            {
                return a_Container.front();
            }
        
            static constexpr const ElementType& Front(const Type& a_Container)
            {
                return a_Container.front();
            }
        
            static constexpr ElementType& Back(Type& a_Container)
            {
                return a_Container.back();
            }
        
            static constexpr const ElementType& Back(const Type& a_Container)
            {
                return a_Container.back();
            }
        
            static constexpr ElementType* Data(Type& a_Container)
            {
                return a_Container.data();
            }
        
            static constexpr const ElementType* Data(const Type& a_Container)
            {
                return a_Container.data();
            }
        
            // Iterators
        
            static constexpr Iter Begin(Type& a_Container) { return a_Container.begin(); }
            static constexpr ConstIter Begin(const Type& a_Container) { return a_Container.begin(); }
        
            static constexpr Iter End(Type& a_Container) { return a_Container.end(); }
            static constexpr ConstIter End(const Type& a_Container) { return a_Container.end(); }
        
            // Reverse Iterators
        
            static constexpr RIter RBegin(Type& a_Container) { return a_Container.rbegin(); }
            static constexpr ConstRIter RBegin(const Type& a_Container) { return a_Container.rbegin(); }
        
            static constexpr RIter REnd(Type& a_Container) { return a_Container.rend(); }
            static constexpr ConstRIter REnd(const Type& a_Container) { return a_Container.rend(); }
        
            // Modification
        
            static constexpr void PushBack(Type& a_Container, const ElementType& a_Element)
            {
                a_Container.push_back(a_Element);
            }
        
            static constexpr void PushBack(Type& a_Container, ElementType&& a_Element)
            {
                a_Container.push_back(std::move(a_Element));
            }
        
            template<typename... Args>
            static constexpr ElementType& EmplaceBack(Type& a_Container, Args&&... a_Args)
            {
                return a_Container.emplace_back(std::forward<Args>(a_Args)...);
            }
        
            static constexpr void PopBack(Type& a_Container)
            {
                a_Container.pop_back();
            }
        
            template<typename U>
            static constexpr Iter Insert( Type& a_Container, ConstIter a_Pos, U&& a_Element )
            {
                return a_Container.insert( a_Pos, std::forward<U>( a_Element ) );
            }
        
            static constexpr Iter Erase(Type& a_Container, ConstIter a_Pos)
            {
                return a_Container.erase(a_Pos);
            }
        };
    }

#endif // Default to std::vector if no custom array implementation is provided.

#ifndef RATUI_OVERRIDE_FIXED_ARRAY_IMPL
    #include <array>

    namespace RatUI
    {
        template<typename T, size_t N>
        using FixedArrayImpl = std::array<T, N>;

        template<typename _ElementType, size_t N>
        struct CoreTraits<FixedArrayImpl<_ElementType, N>>
        {
            using Type        = FixedArrayImpl<_ElementType, N>;
            using ElementType = typename Type::value_type;
            using SizeType    = typename Type::size_type;
            using Iter        = typename Type::iterator;
            using ConstIter   = typename Type::const_iterator;
            using RIter       = typename Type::reverse_iterator;
            using ConstRIter  = typename Type::const_reverse_iterator;

            static constexpr SizeType FixedSize = N;

            // Capacity

            static constexpr SizeType Size(const Type& a_Container)
            {
                return a_Container.size();
            }

            static constexpr bool Empty(const Type& a_Container)
            {
                return a_Container.empty();
            }

            // Element access

            static constexpr ElementType& RawAt(Type& a_Container, SizeType a_Index)
            {
                return a_Container[a_Index];
            }

            static constexpr const ElementType& RawAt(const Type& a_Container, SizeType a_Index)
            {
                return a_Container[a_Index];
            }

            static constexpr ElementType& At(Type& a_Container, SizeType a_Index)
            {
                return a_Container.at(a_Index);
            }

            static constexpr const ElementType& At(const Type& a_Container, SizeType a_Index)
            {
                return a_Container.at(a_Index);
            }

            static constexpr ElementType& Front(Type& a_Container)
            {
                return a_Container.front();
            }

            static constexpr const ElementType& Front(const Type& a_Container)
            {
                return a_Container.front();
            }

            static constexpr ElementType& Back(Type& a_Container)
            {
                return a_Container.back();
            }

            static constexpr const ElementType& Back(const Type& a_Container)
            {
                return a_Container.back();
            }

            static constexpr ElementType* Data(Type& a_Container)
            {
                return a_Container.data();
            }

            static constexpr const ElementType* Data(const Type& a_Container)
            {
                return a_Container.data();
            }

            // Iterators

            static constexpr Iter Begin(Type& a_Container) { return a_Container.begin(); }
            static constexpr ConstIter Begin(const Type& a_Container) { return a_Container.begin(); }

            static constexpr Iter End(Type& a_Container) { return a_Container.end(); }
            static constexpr ConstIter End(const Type& a_Container) { return a_Container.end(); }

            // Reverse Iterators

            static constexpr RIter RBegin(Type& a_Container) { return a_Container.rbegin(); }
            static constexpr ConstRIter RBegin(const Type& a_Container) { return a_Container.rbegin(); }

            static constexpr RIter REnd(Type& a_Container) { return a_Container.rend(); }
            static constexpr ConstRIter REnd(const Type& a_Container) { return a_Container.rend(); }
        };
    }

#endif // Default to std::array if no custom fixed array implementation is provided.

#ifndef RATUI_OVERRIDE_VARIANT_IMPL
    #include <variant>

    namespace RatUI
    {
        template<typename... Types>
        using VariantImpl = std::variant<Types...>;
        using MonostateImpl = std::monostate;
        
        template<typename... Types>
        struct CoreTraits<VariantImpl<Types...>>
        {
            using Type = VariantImpl<Types...>;
            using SizeType = size_t;
            
            template<SizeType I>
			using ElementType = std::variant_alternative_t<I, Type>;

            static constexpr SizeType Index(const Type& a_Variant)
            {
                return a_Variant.index();
			}

            template<typename T>
            static constexpr bool Holds(const Type& a_Variant)
            {
                return std::holds_alternative<T>(a_Variant);
            }

            template<SizeType I>
            static constexpr bool Holds(const Type& a_Variant)
            {
				return std::holds_alternative<ElementType<I>>( a_Variant );
            }

            template<typename T>
            static constexpr T& Get(Type& a_Variant)
            {
                return std::get<T>(a_Variant);
            }

            template<typename T>
            static constexpr const T& Get(const Type& a_Variant)
            {
                return std::get<T>(a_Variant);
            }

            template<SizeType I>
            static constexpr auto& Get(Type& a_Variant)
            {
                return std::get<I>(a_Variant);
            }

            template<SizeType I>
            static constexpr const auto& Get(const Type& a_Variant)
            {
                return std::get<I>(a_Variant);
            }
        };
    }

#endif // Default to std::variant if no custom variant implementation is provided.

#ifndef RATUI_OVERRIDE_OPTIONAL_IMPL
    #include <optional>

    namespace RatUI
    {
        template<typename T>
        using OptionalImpl = std::optional<T>;
        inline constexpr auto NullOptImpl = std::nullopt;

        template<typename T>
        struct CoreTraits<OptionalImpl<T>>
        {
            using Type = OptionalImpl<T>;
            using ValueType = typename Type::value_type;

            static constexpr bool HasValue(const Type& a_Optional)
            {
                return a_Optional.has_value();
            }

            static constexpr ValueType& Value(Type& a_Optional)
            {
                return a_Optional.value();
            }

            static constexpr const ValueType& Value(const Type& a_Optional)
            {
                return a_Optional.value();
            }
        };
    }

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
    using Span = SpanImpl<T>;

    /** 
     * @brief Array is a dynamically sized array container.
     * It is implemented using std::vector by default, but can be customized by defining RATUI_ARRAY_IMPL before including this header.
     * @tparam T The type of elements stored in the array.
     */ 
    template<typename T>
    using Array = ArrayImpl<T>;

    /** 
     * @brief FixedArray is a statically sized array container.
     * It is implemented using std::array by default, but can be customized by defining RATUI_FIXED_ARRAY_IMPL before including this header.
     * @tparam T The type of elements stored in the array.
     * @tparam N The number of elements in the array.
     */
    template<typename T, size_t N>
    using FixedArray = FixedArrayImpl<T, N>;

    /**
     * @brief Variant is a type that can hold one of several alternative types, similar to std::variant.
     * @tparam Types The list of types that the variant can hold.
     */
    template<typename... Types>
    using Variant = VariantImpl<Types...>;

    /** @brief Monostate is a type used to represent an empty state in a Variant, similar to std::monostate. */
    using Monostate = MonostateImpl;

    /** 
     * @brief Optional is a type that may contain a value of type T or be empty, similar to std::optional.
     * @tparam T The type of the value that may be contained in the optional.
     */
    template<typename T>
    using Optional = OptionalImpl<T>;
    inline constexpr auto NullOpt = NullOptImpl;

    // === Container Access ===

    template<typename Container>
    constexpr decltype(auto) Begin(Container& a_Container)
    {
        return CoreTraits<Container>::Begin(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Begin(const Container& a_Container)
    {
        return CoreTraits<Container>::Begin(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) End(Container& a_Container)
    {
        return CoreTraits<Container>::End(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) End(const Container& a_Container)
    {
        return CoreTraits<Container>::End(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) RBegin(Container& a_Container)
    {
        return CoreTraits<Container>::RBegin(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) RBegin(const Container& a_Container)
    {
        return CoreTraits<Container>::RBegin(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) REnd(Container& a_Container)
    {
        return CoreTraits<Container>::REnd(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) REnd(const Container& a_Container)
    {
        return CoreTraits<Container>::REnd(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Size(const Container& a_Container)
    {
        return CoreTraits<Container>::Size(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Empty(const Container& a_Container)
    {
        return CoreTraits<Container>::Empty(a_Container);
    }

    // === Container Modification ===

    template<typename Container, typename... Args>
    constexpr decltype(auto) PushBack(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::PushBack(a_Container, std::forward<Args>(a_Args)...);
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) EmplaceBack(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::EmplaceBack(a_Container, std::forward<Args>(a_Args)...);
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Insert(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::Insert(a_Container, std::forward<Args>(a_Args)...);
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Erase(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::Erase(a_Container, std::forward<Args>(a_Args)...);
    }

    template<typename Container>
    constexpr void PopBack(Container& a_Container)
    {
        CoreTraits<Container>::PopBack(a_Container);
    }

    template<typename Container>
    constexpr void Clear(Container& a_Container)
    {
        CoreTraits<Container>::Clear(a_Container);
    }

    template<typename Container>
    constexpr void Reserve(Container& a_Container, const size a_Capacity)
    {
        CoreTraits<Container>::Reserve(a_Container, a_Capacity);
    }

    template<typename Container>
    constexpr void Resize(Container& a_Container, const size a_Size)
    {
        CoreTraits<Container>::Resize(a_Container, a_Size);
    }

    // === Element Access ===

    template<typename Container>
    constexpr decltype(auto) Front(Container& a_Container)
    {
        return CoreTraits<Container>::Front(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Front(const Container& a_Container)
    {
        return CoreTraits<Container>::Front(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Back(Container& a_Container)
    {
        return CoreTraits<Container>::Back(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Back(const Container& a_Container)
    {
        return CoreTraits<Container>::Back(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) RawAt( Container& a_Container, const size a_Index)
    {
        return CoreTraits<Container>::RawAt(a_Container, a_Index);
	}

    template<typename Container>
    constexpr decltype( auto ) RawAt( const Container& a_Container, const size a_Index )
    {
        return CoreTraits<Container>::RawAt( a_Container, a_Index );
    }

    template<typename Container>
    constexpr decltype(auto) At(Container& a_Container, const size a_Index)
    {
        return CoreTraits<Container>::At(a_Container, a_Index);
    }

    template<typename Container>
    constexpr decltype(auto) At(const Container& a_Container, const size a_Index)
    {
        return CoreTraits<Container>::At(a_Container, a_Index);
    }

    template<typename Container>
    constexpr decltype(auto) Data(Container& a_Container)
    {
        return CoreTraits<Container>::Data(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Data(const Container& a_Container)
    {
        return CoreTraits<Container>::Data(a_Container);
    }

    // === Variant Access ===

	template<typename Container>
    constexpr decltype( auto ) Index( const Container& a_Container )
    {
        return CoreTraits<Container>::Index( a_Container );
	}

    template<typename T, typename Container>
    constexpr bool Holds( const Container& a_Container )
    {
        return CoreTraits<Container>::template Holds<T>( a_Container );
    }

    template<auto I, typename Container>
    constexpr bool Holds( const Container& a_Container )
    {
        return CoreTraits<Container>::template Holds<I>( a_Container );
    }

    template<typename T, typename Container>
    constexpr decltype( auto ) Get( Container&& a_Container )
    {
        return CoreTraits<std::remove_cvref_t<Container>>::template Get<T>( std::forward<Container>( a_Container ) );
    }

	template<auto I, typename Container>
    constexpr decltype( auto ) Get( Container&& a_Container )
    {
        return CoreTraits<std::remove_cvref_t<Container>>::template Get<I>( std::forward<Container>( a_Container ) );
	}

    // === Optional Access ===

    template<typename Container>
    constexpr bool HasValue(const Container& a_Container)
    {
        return CoreTraits<Container>::HasValue(a_Container);
    }

    template<typename Container>
    constexpr decltype(auto) Value(Container&& a_Container)
    {
        using DecayedContainer = std::remove_cvref_t<Container>;
        return CoreTraits<DecayedContainer>::Value(std::forward<Container>(a_Container));
    }

} // namespace RatUI
