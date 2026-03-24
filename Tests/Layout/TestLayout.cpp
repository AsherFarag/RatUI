/**
 * @file TestLayout.cpp
 * @brief Tests for RatUI layout types: EAlignment, ELayoutType, Visibility, Constraints, Geometry.
 */

#include "../Common/Common.h"

using namespace RatUI;

// =============================================================================
// EAlignment
// =============================================================================

TEST_CASE( "EAlignment single horizontal flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( Left    & HCenter ) == 0 );
    REQUIRE( ( Left    & Right   ) == 0 );
    REQUIRE( ( HCenter & Right   ) == 0 );
}

TEST_CASE( "EAlignment single vertical flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( Top     & VCenter ) == 0 );
    REQUIRE( ( Top     & Bottom  ) == 0 );
    REQUIRE( ( VCenter & Bottom  ) == 0 );
}

TEST_CASE( "EAlignment combined presets match their component flags", "[layout][alignment]" )
{
    REQUIRE( TopLeft      == ( Top     | Left    ) );
    REQUIRE( TopCenter    == ( Top     | HCenter ) );
    REQUIRE( TopRight     == ( Top     | Right   ) );
    REQUIRE( CenterLeft   == ( VCenter | Left    ) );
    REQUIRE( Center       == ( VCenter | HCenter ) );
    REQUIRE( CenterRight  == ( VCenter | Right   ) );
    REQUIRE( BottomLeft   == ( Bottom  | Left    ) );
    REQUIRE( BottomCenter == ( Bottom  | HCenter ) );
    REQUIRE( BottomRight  == ( Bottom  | Right   ) );
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