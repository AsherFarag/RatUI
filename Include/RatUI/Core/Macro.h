#pragma once

/** @brief Helper macro to conditionally compile and return an expression if it is valid. */
#define RATUI_TRY_EXPR( _Expr ) \
    if constexpr ( requires { _Expr; } ) \
    { \
        return _Expr; \
    }

#define RATUI_ENUM_ENABLE_BITMASK_OPERATORS( EnumName, UnderlyingType ) \
    inline constexpr EnumName operator|( EnumName a, EnumName b ) \
    { \
        return static_cast<EnumName>( static_cast<UnderlyingType>( a ) | static_cast<UnderlyingType>( b ) ); \
    } \
    inline constexpr EnumName operator&( EnumName a, EnumName b ) \
    { \
        return static_cast<EnumName>( static_cast<UnderlyingType>( a ) & static_cast<UnderlyingType>( b ) ); \
    } \
    inline constexpr EnumName operator^( EnumName a, EnumName b ) \
    { \
        return static_cast<EnumName>( static_cast<UnderlyingType>( a ) ^ static_cast<UnderlyingType>( b ) ); \
    } \
    inline constexpr EnumName operator~( EnumName a ) \
    { \
        return static_cast<EnumName>( ~static_cast<UnderlyingType>( a ) ); \
    } \
    inline constexpr EnumName& operator|=( EnumName& a, EnumName b ) \
    { \
        return a = a | b; \
    } \
    inline constexpr EnumName& operator&=( EnumName& a, EnumName b ) \
    { \
        return a = a & b; \
    } \
    inline constexpr EnumName& operator^=( EnumName& a, EnumName b ) \
    { \
        return a = a ^ b; \
    }
    