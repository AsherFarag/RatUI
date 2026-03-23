/**
 * @file TestLayout.cpp
 * @brief Tests for RatUI layout types: EAlignment, ELayoutDirection, Visibility, Constraints, Geometry.
 */

#include "../Common/Common.h"

using namespace RatUI;

// =============================================================================
// EAlignment
// =============================================================================

TEST_CASE( "EAlignment single horizontal flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( AlignLeft   & AlignHCenter ) == 0 );
    REQUIRE( ( AlignLeft   & AlignRight   ) == 0 );
    REQUIRE( ( AlignHCenter & AlignRight  ) == 0 );
}

TEST_CASE( "EAlignment single vertical flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( AlignTop    & AlignVCenter ) == 0 );
    REQUIRE( ( AlignTop    & AlignBottom  ) == 0 );
    REQUIRE( ( AlignVCenter & AlignBottom ) == 0 );
}

TEST_CASE( "EAlignment combined presets match their component flags", "[layout][alignment]" )
{
    REQUIRE( AlignTopLeft     == ( AlignTop    | AlignLeft    ) );
    REQUIRE( AlignTopCenter   == ( AlignTop    | AlignHCenter ) );
    REQUIRE( AlignTopRight    == ( AlignTop    | AlignRight   ) );
    REQUIRE( AlignCenterLeft  == ( AlignVCenter | AlignLeft   ) );
    REQUIRE( AlignCenter      == ( AlignVCenter | AlignHCenter) );
    REQUIRE( AlignCenterRight == ( AlignVCenter | AlignRight  ) );
    REQUIRE( AlignBottomLeft  == ( AlignBottom | AlignLeft    ) );
    REQUIRE( AlignBottomCenter== ( AlignBottom | AlignHCenter ) );
    REQUIRE( AlignBottomRight == ( AlignBottom | AlignRight   ) );
}

// =============================================================================
// ELayoutDirection
// =============================================================================

TEST_CASE( "ELayoutDirection values are distinct", "[layout][direction]" )
{
    REQUIRE( ELayoutDirection::Horizontal != ELayoutDirection::Vertical );
    REQUIRE( ELayoutDirection::Horizontal != ELayoutDirection::Stack     );
    REQUIRE( ELayoutDirection::Horizontal != ELayoutDirection::Grid      );
    REQUIRE( ELayoutDirection::Vertical   != ELayoutDirection::Stack     );
    REQUIRE( ELayoutDirection::Vertical   != ELayoutDirection::Grid      );
    REQUIRE( ELayoutDirection::Stack      != ELayoutDirection::Grid      );
}

// =============================================================================
// Visibility
// =============================================================================

TEST_CASE( "Visibility defaults to Visible", "[layout][visibility]" )
{
    Visibility v;
    REQUIRE( v.IsVisible() );
    REQUIRE( v.Value == Visibility::Visible );
}

TEST_CASE( "Visibility Hidden is not visible", "[layout][visibility]" )
{
    Visibility v{ Visibility::Hidden };
    REQUIRE_FALSE( v.IsVisible() );
}

TEST_CASE( "Visibility Collapsed is not visible", "[layout][visibility]" )
{
    Visibility v{ Visibility::Collapsed };
    REQUIRE_FALSE( v.IsVisible() );
}

// =============================================================================
// Constraints
// =============================================================================

TEST_CASE( "Constraints::Unbounded has zero min and max float max", "[layout][constraints]" )
{
    Constraints c = Constraints::Unbounded();
    REQUIRE( c.MinSize[ 0 ] == 0.0f );
    REQUIRE( c.MinSize[ 1 ] == 0.0f );
    REQUIRE( c.MaxSize[ 0 ] == std::numeric_limits<f32>::max() );
    REQUIRE( c.MaxSize[ 1 ] == std::numeric_limits<f32>::max() );
}

TEST_CASE( "Constraints::Fixed sets min and max to the same size", "[layout][constraints]" )
{
    Constraints c = Constraints::Fixed( Vec2f( 100.0f, 200.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 100.0f, 200.0f ) );
    RequireApproxEqual( c.MaxSize, Vec2f( 100.0f, 200.0f ) );
}

TEST_CASE( "Constraints::AtLeast sets min and leaves max unbounded", "[layout][constraints]" )
{
    Constraints c = Constraints::AtLeast( Vec2f( 50.0f, 75.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 50.0f, 75.0f ) );
    REQUIRE( c.MaxSize[ 0 ] == std::numeric_limits<f32>::max() );
    REQUIRE( c.MaxSize[ 1 ] == std::numeric_limits<f32>::max() );
}

TEST_CASE( "Constraints::AtMost leaves min at zero and sets max", "[layout][constraints]" )
{
    Constraints c = Constraints::AtMost( Vec2f( 300.0f, 400.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( c.MaxSize, Vec2f( 300.0f, 400.0f ) );
}

// =============================================================================
// LayoutInput
// =============================================================================

TEST_CASE( "LayoutInput has correct defaults", "[layout][input]" )
{
    LayoutInput input;
    REQUIRE( input.Padding[ 0 ]   == 0.0f );
    REQUIRE( input.Margin[ 0 ]    == 0.0f );
    REQUIRE( input.SizeHint[ 0 ]  == 0.0f );
    REQUIRE( input.SizeHint[ 1 ]  == 0.0f );
    REQUIRE( input.FlexWeight     == 0.0f );
    REQUIRE( input.Alignment      == AlignTopLeft );
    REQUIRE( input.LayoutDirection == ELayoutDirection::Horizontal );
}

// =============================================================================
// Geometry
// =============================================================================

TEST_CASE( "Geometry has correct defaults", "[layout][geometry]" )
{
    Geometry g;
    RequireApproxEqual( g.LocalPosition,    Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( g.LocalSize,        Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( g.AbsoluteScale,    Vec2f( 1.0f, 1.0f ) );
    RequireApproxEqual( g.AbsolutePosition, Vec2f( 0.0f, 0.0f ) );
}
