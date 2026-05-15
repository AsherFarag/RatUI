#pragma once

/**
 * @file Common.h
 * @brief Shared test utilities and helpers for RatUI tests.
 */

#include <RatUI/RatUI.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Approximate float comparison tolerance used throughout the test suite.
inline constexpr float k_FloatEpsilon = 1e-5f;

inline float ToFloat( RatUI::Unit a_Value ) { return a_Value.ToFloat(); }
inline RatUI::Unit ToUnit( float a_Value ) { return RatUI::Unit{ a_Value }; }

inline RatUI::Vec2<RatUI::Unit> ToUnitVec2( const RatUI::Vec2f& a_Value )
{
    return { ToUnit( a_Value[ 0 ] ), ToUnit( a_Value[ 1 ] ) };
}

inline RatUI::Rect<RatUI::Unit> ToUnitRect( const RatUI::Rectf& a_Value )
{
    return { ToUnitVec2( a_Value.Origin ), ToUnitVec2( a_Value.Size ) };
}

inline RatUI::Vec2f ToFloatVec2( const RatUI::Vec2<RatUI::Unit>& a_Value )
{
    return { a_Value[ 0 ].ToFloat(), a_Value[ 1 ].ToFloat() };
}

inline RatUI::Rectf ToFloatRect( const RatUI::Rect<RatUI::Unit>& a_Value )
{
    return { ToFloatVec2( a_Value.Origin ), ToFloatVec2( a_Value.Size ) };
}

/** @brief Checks that two Vec2f values are approximately equal component-wise. */
inline void RequireApproxEqual( const RatUI::Vec2f& a_Lhs, const RatUI::Vec2f& a_Rhs )
{
    REQUIRE( a_Lhs[ 0 ] == Catch::Approx( a_Rhs[ 0 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 1 ] == Catch::Approx( a_Rhs[ 1 ] ).epsilon( k_FloatEpsilon ) );
}

inline void RequireApproxEqual( const RatUI::Vec2<RatUI::Unit>& a_Lhs, const RatUI::Vec2f& a_Rhs )
{
    RequireApproxEqual( ToFloatVec2( a_Lhs ), a_Rhs );
}

inline void RequireApproxEqual( const RatUI::Vec2f& a_Lhs, const RatUI::Vec2<RatUI::Unit>& a_Rhs )
{
    RequireApproxEqual( a_Lhs, ToFloatVec2( a_Rhs ) );
}

/** @brief Checks that two Vec3f values are approximately equal component-wise. */
inline void RequireApproxEqual( const RatUI::Vec3f& a_Lhs, const RatUI::Vec3f& a_Rhs )
{
    REQUIRE( a_Lhs[ 0 ] == Catch::Approx( a_Rhs[ 0 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 1 ] == Catch::Approx( a_Rhs[ 1 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 2 ] == Catch::Approx( a_Rhs[ 2 ] ).epsilon( k_FloatEpsilon ) );
}

inline bool operator==( RatUI::Unit a_Lhs, float a_Rhs )
{
    return a_Lhs.ToFloat() == a_Rhs;
}

inline bool operator==( float a_Lhs, RatUI::Unit a_Rhs )
{
    return a_Lhs == a_Rhs.ToFloat();
}

inline bool operator==( RatUI::Unit a_Lhs, const Catch::Approx& a_Rhs )
{
    return a_Rhs == a_Lhs.ToFloat();
}

inline bool operator==( const Catch::Approx& a_Lhs, RatUI::Unit a_Rhs )
{
    return a_Lhs == a_Rhs.ToFloat();
}
