#pragma once

/** 
 * @file Containers.inl
 * @brief This file contains container type definitions and utility functions for RatUI.
 * It is included by Core.h and should not be included directly by user code.
 * The container types and functions defined here are designed to be flexible and adaptable to different container implementations.
 */

#include "Types.h"
#include <iterator>
#include <utility>

namespace RatUI
{
    namespace Detail
    {
        /** 
         * @brief A fallback type for containers that do not have the expected member types.
         * @example std::unordered_map does not have reverse iterators, so this fallback type provides a default implementation for such cases.
         */
        struct EmptyContainerFallback
        {
            using value_type              = void;
            using size_type               = void;
            using iterator                = void;
            using const_iterator          = void;
            using reverse_iterator        = void;
            using const_reverse_iterator  = void;
        };

    #define RATUI_DETECT_MEMBER_TYPE( _Name, _Member )                                                      \
        template<typename T>                                                                                \
        concept Has##_Name = requires { typename T::_Member; };                                             \
        template<typename T>                                                                                \
        using _Name##Or_t = typename std::conditional_t<Has##_Name<T>, T, EmptyContainerFallback>::_Member;

        RATUI_DETECT_MEMBER_TYPE( ValueType,  value_type )
        RATUI_DETECT_MEMBER_TYPE( SizeType,   size_type )
        RATUI_DETECT_MEMBER_TYPE( Iter,       iterator )
        RATUI_DETECT_MEMBER_TYPE( ConstIter,  const_iterator )
        RATUI_DETECT_MEMBER_TYPE( RIter,      reverse_iterator )
        RATUI_DETECT_MEMBER_TYPE( ConstRIter, const_reverse_iterator )

    #undef RATUI_DETECT_MEMBER_TYPE

    } // namespace Detail

    /**
     * @brief Helper struct to extract member types from standard containers.
     * This struct provides type aliases for common member types found in standard containers.
     * This encompasses all common functionality of standard containers, including size, iteration, and element access.
     */
    template<typename _Container>
    struct StdContainerTraits
    {
        using Type        = _Container;
        using ValueType   = Detail::ValueTypeOr_t<Type>;
        using SizeType    = Detail::SizeTypeOr_t<Type>;
        using Iter        = Detail::IterOr_t<Type>;
        using ConstIter   = Detail::ConstIterOr_t<Type>;
        using RIter       = Detail::RIterOr_t<Type>;
        using ConstRIter  = Detail::ConstRIterOr_t<Type>;

        // --- Size ---

        static constexpr SizeType Size( const Type& a_Container ) 
            requires requires { { a_Container.size() } -> std::convertible_to<SizeType>; }
        {
            return a_Container.size();
        }

        static constexpr SizeType SizeBytes( const Type& a_Container ) 
            requires requires { { a_Container.size_bytes() } -> std::convertible_to<SizeType>; }
        {
            return a_Container.size_bytes();
        }

        static constexpr SizeType Capacity( const Type& a_Container ) 
            requires requires { { a_Container.capacity() } -> std::convertible_to<SizeType>; }
        {
            return a_Container.capacity();
        }

        static constexpr bool Empty( const Type& a_Container ) 
            requires requires { { a_Container.empty() } -> std::convertible_to<bool>; }
        {
            return a_Container.empty();
        }

        // --- Value access ---

        static constexpr ValueType& RawAt( Type& a_Container, SizeType a_Index )
            requires requires { { a_Container[a_Index] } -> std::convertible_to<ValueType&>; }
        {
            return a_Container[a_Index];
        }

        static constexpr const ValueType& RawAt( const Type& a_Container, SizeType a_Index )
            requires requires { { a_Container[a_Index] } -> std::convertible_to<const ValueType&>; }
        {
            return a_Container[a_Index];
        }

        static constexpr ValueType& Front( Type& a_Container )
            requires requires { { a_Container.front() } -> std::convertible_to<ValueType&>; }
        {
            return a_Container.front();
        }

        static constexpr const ValueType& Front( const Type& a_Container )
            requires requires { { a_Container.front() } -> std::convertible_to<const ValueType&>; }
        {
            return a_Container.front();
        }

        static constexpr ValueType& Back( Type& a_Container )
            requires requires { { a_Container.back() } -> std::convertible_to<ValueType&>; }
        {
            return a_Container.back();
        }

        static constexpr const ValueType& Back( const Type& a_Container )
            requires requires { { a_Container.back() } -> std::convertible_to<const ValueType&>; }
        {
            return a_Container.back();
        }

        static constexpr ValueType* Data( Type& a_Container )
            requires requires { { a_Container.data() } -> std::convertible_to<ValueType*>; }
        {
            return a_Container.data();
        }

        static constexpr const ValueType* Data( const Type& a_Container )
            requires requires { { a_Container.data() } -> std::convertible_to<const ValueType*>; }
        {
            return a_Container.data();
        }

        // --- Iterators ---

        static constexpr Iter Begin( Type& a_Container ) 
            requires requires { { a_Container.begin() } -> std::convertible_to<Iter>; }
        {
            return a_Container.begin();
        }

        static constexpr ConstIter Begin( const Type& a_Container ) 
            requires requires { { a_Container.begin() } -> std::convertible_to<ConstIter>; }
        {
            return a_Container.begin();
        }

        static constexpr Iter End( Type& a_Container ) 
            requires requires { { a_Container.end() } -> std::convertible_to<Iter>; }
        {
            return a_Container.end();
        }

        static constexpr ConstIter End( const Type& a_Container ) 
            requires requires { { a_Container.end() } -> std::convertible_to<ConstIter>; }
        {
            return a_Container.end();
        }

        static constexpr RIter RBegin( Type& a_Container ) 
            requires requires { { a_Container.rbegin() } -> std::convertible_to<RIter>; }
        {
            return a_Container.rbegin();
        }

        static constexpr ConstRIter RBegin( const Type& a_Container ) 
            requires requires { { a_Container.rbegin() } -> std::convertible_to<ConstRIter>; }
        {
            return a_Container.rbegin();
        }

        static constexpr RIter REnd( Type& a_Container ) 
            requires requires { { a_Container.rend() } -> std::convertible_to<RIter>; }
        {
            return a_Container.rend();
        }

        static constexpr ConstRIter REnd( const Type& a_Container ) 
            requires requires { { a_Container.rend() } -> std::convertible_to<ConstRIter>; }
        {
            return a_Container.rend();
        }

        // --- Key/Index access ---
        // Generic over both sequence containers (SizeType index) and associative
        // containers (KeyType key), since both are ultimately just "whatever
        // operator[]/.at() accepts" on the underlying std container.

        template<typename K>
        static constexpr decltype(auto) At( Type& a_Container, const K& a_Key )
            requires requires { a_Container.at( a_Key ); }
        {
            return a_Container.at( a_Key );
        }

        template<typename K>
        static constexpr decltype(auto) At( const Type& a_Container, const K& a_Key )
            requires requires { a_Container.at( a_Key ); }
        {
            return a_Container.at( a_Key );
        }

        template<typename K>
        static constexpr decltype(auto) RawAt( Type& a_Container, const K& a_Key )
            requires requires { a_Container[a_Key]; }
        {
            return a_Container[a_Key];
        }

        // --- Modification ---

        static constexpr void PushBack( Type& a_Container, const ValueType& a_Element )
            requires requires { a_Container.push_back( a_Element ); }
        {
            a_Container.push_back( a_Element );
        }

        static constexpr void PushBack( Type& a_Container, ValueType&& a_Element )
            requires requires { a_Container.push_back( std::move( a_Element ) ); }
        {
            a_Container.push_back( std::move( a_Element ) );
        }

        template<typename... Args>
        static constexpr decltype(auto) Emplace( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.emplace( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.emplace( std::forward<Args>( a_Args )... );
        }

        template<typename... Args>
        static constexpr decltype(auto) EmplaceBack( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.emplace_back( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.emplace_back( std::forward<Args>( a_Args )... );
        }

        template<typename... Args>
        static constexpr decltype(auto) TryEmplace( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.try_emplace( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.try_emplace( std::forward<Args>( a_Args )... );
        }

        template<typename... Args>
        static constexpr decltype(auto) Insert( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.insert( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.insert( std::forward<Args>( a_Args )... );
        }

        template<typename... Args>
        static constexpr decltype(auto) Erase( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.erase( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.erase( std::forward<Args>( a_Args )... );
        }

        static constexpr void PopBack( Type& a_Container )
            requires requires { a_Container.pop_back(); }
        {
            a_Container.pop_back();
        }

        static constexpr void Clear( Type& a_Container )
            requires requires { a_Container.clear(); }
        {
            a_Container.clear();
        }

        static constexpr void Reserve( Type& a_Container, SizeType a_Count )
            requires requires { a_Container.reserve( a_Count ); }
        {
            a_Container.reserve( a_Count );
        }

        static constexpr void Resize( Type& a_Container, SizeType a_Count )
            requires requires { a_Container.resize( a_Count ); }
        {
            a_Container.resize( a_Count );
        }

        // --- Lookup ---

        template<typename... Args>
        static constexpr decltype(auto) Find( Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.find( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.find( std::forward<Args>( a_Args )... );
        }

        template<typename... Args>
        static constexpr decltype(auto) Find( const Type& a_Container, Args&&... a_Args )
            requires requires { a_Container.find( std::forward<Args>( a_Args )... ); }
        {
            return a_Container.find( std::forward<Args>( a_Args )... );
        }

        template<typename K>
        static constexpr bool Contains( const Type& a_Container, const K& a_Key )
            requires requires { { a_Container.contains( a_Key ) } -> std::convertible_to<bool>; }
        {
            return a_Container.contains( a_Key );
        }

        template<typename K>
        static constexpr bool Contains( const Type& a_Container, const K& a_Key )
            requires ( !requires { a_Container.contains( a_Key ); } &&
                       requires { { a_Container.find( a_Key ) } -> std::convertible_to<ConstIter>; } )
        {
            return a_Container.find( a_Key ) != a_Container.end();
        }
    };


} // namespace RatUI

#if defined(__has_include)
    #if __has_include("RatUIContainerImpl.h")
        #include "RatUIContainerImpl.h"
    #elif __has_include(<RatUIContainerImpl.h>)
        #include <RatUIContainerImpl.h>
    #endif
#endif

#ifndef RATUI_OVERRIDE_SPAN_IMPL

#include <span>

namespace RatUI
{
    template<typename T>
    using SpanImpl = std::span<T>;

    template<typename _ElementType>
    struct CoreTraits<SpanImpl<_ElementType>> : StdContainerTraits<SpanImpl<_ElementType>>
    {
        using Type        = SpanImpl<_ElementType>;
        using ElementType = typename StdContainerTraits<Type>::ValueType;

        static constexpr typename StdContainerTraits<Type>::SizeType DynamicExtent = std::dynamic_extent;
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
    struct CoreTraits<ArrayImpl<_ElementType>> : StdContainerTraits<ArrayImpl<_ElementType>>
    {
        using Type        = ArrayImpl<_ElementType>;
        using ElementType = typename StdContainerTraits<Type>::ValueType;
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
    struct CoreTraits<FixedArrayImpl<_ElementType, N>> : StdContainerTraits<FixedArrayImpl<_ElementType, N>>
    {
        using Type        = FixedArrayImpl<_ElementType, N>;
        using ElementType = typename StdContainerTraits<Type>::ValueType;

        static constexpr typename StdContainerTraits<Type>::SizeType FixedSize = N;
    };
}

#endif // Default to std::array if no custom fixed array implementation is provided.


#ifndef RATUI_OVERRIDE_HASHMAP_IMPL
#include <unordered_map>

namespace RatUI
{
    template<typename Key, typename Value,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>>
    using HashMapImpl = std::unordered_map<Key, Value, Hash, KeyEqual>;

    template<typename Key, typename Value, typename Hash, typename KeyEqual>
    struct CoreTraits<HashMapImpl<Key, Value, Hash, KeyEqual>> : StdContainerTraits<HashMapImpl<Key, Value, Hash, KeyEqual>>
    {
        using Type       = HashMapImpl<Key, Value, Hash, KeyEqual>;
        using KeyType    = typename Type::key_type;
        using MappedType = typename Type::mapped_type;
    };
}

#endif // Default to std::unordered_map if no custom hashmap implementation is provided.

#ifndef RATUI_OVERRIDE_STRING_IMPL
#include <string>

namespace RatUI
{
    using StringImpl = std::string;

    template<>
    struct CoreTraits<StringImpl> : StdContainerTraits<StringImpl>
    {
        using Type        = StringImpl;
        using CharType    = typename Type::value_type;
        using CharTraits  = typename Type::traits_type;
        using ElementType = CharType;

        static constexpr const CharType* CStr( const Type& a_String )
        {
            return a_String.c_str();
        }
    };
}

#endif // Default to std::string if no custom string implementation is provided.

#ifndef RATUI_STRING_VIEW_IMPL
#include <string_view>

namespace RatUI
{
    using StringViewImpl = std::string_view;

    template<>
    struct CoreTraits<StringViewImpl> : StdContainerTraits<StringViewImpl>
    {
        using Type        = StringViewImpl;
        using CharType    = typename Type::value_type;
        using CharTraits  = typename Type::traits_type;
        using ElementType = CharType;
    };
}

#endif // Default to std::string_view if no custom string view implementation is provided.

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
		using TypeAt = std::variant_alternative_t<I, Type>;

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
			return std::holds_alternative<TypeAt<I>>( a_Variant );
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
     * @brief HashMap is a hash table based associative container that contains key-value pairs with unique keys.
     * It is implemented using std::unordered_map by default, but can be customized by defining RATUI_HASHMAP_IMPL before including this header.
     * @tparam Key The type of the keys in the map.
     * @tparam Value The type of the values in the map.
     * @tparam Hash The type of the hash function used to hash the keys. Defaults to std::hash<Key>.
     * @tparam KeyEqual The type of the equality function used to compare keys. Defaults to std::equal_to<Key>.
     */
    template<typename Key, typename Value, typename Hash = HashMapImpl<Key, Value>::hasher, typename KeyEqual = HashMapImpl<Key, Value>::key_equal>
    using HashMap = HashMapImpl<Key, Value, Hash, KeyEqual>;

    /**
     * @brief String is a dynamic array of characters
     */
    using String = StringImpl;

    /**
     * @brief StringView is a non-owning view over a string, useful for read-only access to string data without copying.
     */
    using StringView = StringViewImpl;

    /** 
     * @brief CStr returns a pointer to the underlying character array of the string, suitable for interop with C-style APIs.
     * @note The string must be null-terminated.
     */
    inline const char* CStr( const String& a_String ) { return CoreTraits<String>::CStr( a_String ); }

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
	constexpr decltype( auto ) SizeBytes( const Container& a_Container )
	{
		return CoreTraits<Container>::SizeBytes( a_Container );
	}

    template<typename Container>
    constexpr decltype(auto) Capacity(const Container& a_Container)
    {
        return CoreTraits<Container>::Capacity(a_Container);
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
    constexpr decltype(auto) Emplace(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::Emplace(a_Container, std::forward<Args>(a_Args)...);
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
    constexpr decltype(auto) RawAt( const Container& a_Container, const size a_Index )
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

    template<typename Container, typename... Args>
    constexpr decltype(auto) Find(Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::Find(a_Container, std::forward<decltype(a_Args)>(a_Args)...);
    }

    template<typename Container, typename... Args>
    constexpr decltype(auto) Find(const Container& a_Container, Args&&... a_Args)
    {
        return CoreTraits<Container>::Find(a_Container, std::forward<decltype(a_Args)>(a_Args)...);
    }

    // === Variant Access ===

	template<typename Container>
    constexpr decltype(auto) Index( const Container& a_Container )
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
    constexpr decltype(auto) Get( Container&& a_Container )
    {
        return CoreTraits<std::remove_cvref_t<Container>>::template Get<T>( std::forward<Container>( a_Container ) );
    }

	template<auto I, typename Container>
    constexpr decltype(auto) Get( Container&& a_Container )
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
