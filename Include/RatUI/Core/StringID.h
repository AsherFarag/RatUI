#pragma once
#include "Types.h"

namespace RatUI
{
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
            return static_cast<size_t>( a_ID.Hash );
        }
    };

    namespace Literals
    {
        /**
         * @brief User-defined literal for creating StringIDs from string literals. Usage: "MyString"_id
         */
        constexpr StringID operator""_id( const char* a_String, size_t )
        {
            return StringID( a_String );
        }
    } // namespace Literals

} // namespace RatUI