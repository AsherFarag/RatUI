#pragma once
#include "Config.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

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

    using byte = std::byte;

    /** 
     * @brief Type for representing a Unicode code point. 
     * This is a 32-bit type that can represent any Unicode code point, 
     * including those outside the Basic Multilingual Plane (BMP). 
     */
    using codepoint = c32;

    /** Shorthand because I'm lazy and sick of typing std::numeric_limits<T>::max() all the time. */
    template<typename T>
    using Limits = std::numeric_limits<T>;

    template<typename>
    inline constexpr bool AlwaysFalse = false;

	template<typename T>
    inline constexpr bool HasFlag( T a_Value, T a_Flag )
    {
        return ( a_Value & a_Flag ) == a_Flag;
    }

    template<typename T>
    inline constexpr auto ToUnderlying( T a_Enum ) -> std::underlying_type_t<T>
    {
        return static_cast<std::underlying_type_t<T>>( a_Enum );
    }

    /**
     * @brief A unique identifier created from a string hash.
     */
    struct StringID
    {
        u64 Hash{ 0 };

    #if RATUI_DEBUG
        const char* SourceString{ nullptr }; ///< Optional: String the hash was generated from, used for debugging purposes.
    #endif

        constexpr bool operator==( const StringID& a_Other ) const { return Hash == a_Other.Hash; }
        constexpr bool operator!=( const StringID& a_Other ) const { return Hash != a_Other.Hash; }
        constexpr bool operator< ( const StringID& a_Other ) const { return Hash <  a_Other.Hash; }
        constexpr bool operator> ( const StringID& a_Other ) const { return Hash >  a_Other.Hash; }
        constexpr bool operator<=( const StringID& a_Other ) const { return Hash <= a_Other.Hash; }
        constexpr bool operator>=( const StringID& a_Other ) const { return Hash >= a_Other.Hash; }
    
                 constexpr StringID() = default;
        explicit constexpr StringID( u64 a_Hash ) : Hash( a_Hash ) {}
    
        static constexpr u64 HashString( const char* a_String )
        {
            u64 hash = 14695981039346656037ull; // FNV offset basis
        
            while ( const char c = *a_String )
            {
                hash ^= static_cast<u8>( c );
                hash *= 1099511628211ull; // FNV prime
                ++a_String;
            }
        
            // MurmurHash3 fmix64 finalizer
			// FNV-1a is not great at avalanche, so we apply the fmix64 finalizer to improve the distribution of hash values.
            hash ^= hash >> 33;
            hash *= 0xff51afd7ed558ccdULL;
            hash ^= hash >> 33;
            hash *= 0xc4ceb9fe1a85ec53ULL;
            hash ^= hash >> 33;
        
            return hash;
        }
    
        explicit constexpr StringID( const char* a_Name )
            : Hash( HashString( a_Name ) )
    #if RATUI_DEBUG
            , SourceString( a_Name )
    #endif
        {}        
    };

    struct StringIDHash
    {
        size_t operator()( const StringID& a_ID ) const
        {
            return std::hash<u64>{}( a_ID.Hash );
        }
    };

    namespace Literals
    {
        /**
         * @brief User-defined literal for creating StringIDs from string literals. Usage: "MyString"_id
         */
        constexpr StringID operator"" _id( const char* a_String, size_t )
        {
            return StringID( a_String );
        }
    } // namespace Literals

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