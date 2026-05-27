#pragma once
#include "../Core.h"
#include "../Text/Text.h"
#include "../Layout/Layout.h" // TODO: Remove once CornerRounding is moved to a more appropriate header

namespace RatUI
{
    struct ThemeID
    {
        u64 Hash{ 0 };

        constexpr auto operator<=>( const ThemeID& ) const = default;

        constexpr ThemeID() = default;
        constexpr explicit ThemeID( u64 a_Hash ) : Hash( a_Hash ) {}
        constexpr explicit ThemeID( StringView a_Name )
        {
            // Simple FNV-1a hash implementation for generating theme IDs from string names.
            // We avoid using std::hash on strings because it is not guaranteed to be consistent across different runs or platforms,
            // which would make theme IDs unreliable.
            // TODO: Since we prefer safer hashes over speed for theme IDs (the idea is to cache them),
            // investigate other hash functions.
            u64 hash = 14695981039346656037ull; // FNV offset basis
            for ( char c : a_Name )
            {
                hash ^= static_cast<u64>( c );
                hash *= 1099511628211ull; // FNV prime
            }
            Hash = hash;
        }
    };

    namespace Literals
    {
        constexpr ThemeID operator"" _theme( const char* a_String, size_t a_Length )
        {
            return ThemeID( StringView{ a_String, a_Length } );
        }
    } // namespace Literals

    struct ThemeIDHash
    {
        size_t operator()( const ThemeID& a_ID ) const
        {
            return std::hash<u64>{}( a_ID.Hash );
        }
    };
    

    struct Theme
    {
        Weak<Theme> Parent; ///< Optional parent theme to inherit from. If a property is not found in this theme, 
                            ///< it will be looked up in the parent theme.
                  
        HashMap<ThemeID, Color,           ThemeIDHash> Colors;
        HashMap<ThemeID, CornerRounding,  ThemeIDHash> Roundings;
        HashMap<ThemeID, TextRenderStyle, ThemeIDHash> TextStyles;
        HashMap<ThemeID, Unit,            ThemeIDHash> Metrics;

        // TODO: People might not like the use of macros here, but it saves a lot of boilerplate code for these accessors. 
        // Maybe we can find a better way to generate them in the future.
        #define RATUI_THEME_PROPERTY_ACCESSORS( Type, ValueName, MapName ) \
            Optional<Type> TryGet##ValueName( ThemeID a_ID ) const \
            { \
                if ( auto it = Find( MapName, a_ID ); it != End( MapName ) ) \
                    return it->second; \
                \
                if ( auto lock = Parent.lock() ) \
                    return lock->TryGet##ValueName( a_ID ); \
                \
                return NullOpt; \
            } \
            \
            Type Get##ValueName( ThemeID a_ID, Type a_Default ) const \
            { \
                if ( auto it = Find( MapName, a_ID ); it != End( MapName ) ) \
                    return it->second; \
                \
                if ( auto lock = Parent.lock() ) \
                    return lock->Get##ValueName( a_ID, a_Default ); \
                \
                return a_Default; \
            }

        RATUI_THEME_PROPERTY_ACCESSORS( Color,           Color, Colors )
        RATUI_THEME_PROPERTY_ACCESSORS( CornerRounding,  Rounding, Roundings )
        RATUI_THEME_PROPERTY_ACCESSORS( TextRenderStyle, TextStyle, TextStyles )
        RATUI_THEME_PROPERTY_ACCESSORS( Unit,            Metric, Metrics )

        #undef RATUI_THEME_PROPERTY_ACCESSORS
    };

} // namespace RatUI