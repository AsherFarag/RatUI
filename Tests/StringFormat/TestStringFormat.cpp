/**
 * @file TestStringFormat.cpp
 * @brief Tests for RatUI std::formatter specialisations.
 */

#include "../Common/Common.h"
#include "RatUI/RatUI.h"
#include "RatUI/StringFormat.h"
#include <format>
#include <limits>

using namespace RatUI;

// =============================================================================
// Vec formatter
// =============================================================================

TEST_CASE( "Vec2f formats as (x, y)", "[format][vec]" )
{
    Vec2f v( 1.0f, 2.0f );
    REQUIRE( std::format( "{}", v ) == "(1, 2)" );
}

TEST_CASE( "Vec3f formats as (x, y, z)", "[format][vec]" )
{
    Vec3f v( 1.0f, 2.0f, 3.0f );
    REQUIRE( std::format( "{}", v ) == "(1, 2, 3)" );
}

TEST_CASE( "Vec2f zero formats correctly", "[format][vec]" )
{
    Vec2f v;
    REQUIRE( std::format( "{}", v ) == "(0, 0)" );
}

// =============================================================================
// EAlignment formatter
// =============================================================================

TEST_CASE( "Ement formats preset TopLeft", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", TopLeft ) == "TopLeft" );
}

TEST_CASE( "Ement formats preset Center", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", Center ) == "Center" );
}

TEST_CASE( "Ement formats preset BottomRight", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", BottomRight ) == "BottomRight" );
}

TEST_CASE( "Ement formats all nine presets correctly", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", TopLeft     ) == "TopLeft"     );
    REQUIRE( std::format( "{}", TopCenter   ) == "TopCenter"   );
    REQUIRE( std::format( "{}", TopRight    ) == "TopRight"    );
    REQUIRE( std::format( "{}", CenterLeft  ) == "CenterLeft"  );
    REQUIRE( std::format( "{}", Center      ) == "Center"      );
    REQUIRE( std::format( "{}", CenterRight ) == "CenterRight" );
    REQUIRE( std::format( "{}", BottomLeft  ) == "BottomLeft"  );
    REQUIRE( std::format( "{}", BottomCenter) == "BottomCenter");
    REQUIRE( std::format( "{}", BottomRight ) == "BottomRight" );
}

TEST_CASE( "Ement zero value formats as None", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", static_cast<EAlignment>( 0 ) ) == "None" );
}

TEST_CASE( "Ement formats individual flag Left", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", Left ) == "Left" );
}

TEST_CASE( "Ement formats individual flag Top", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", Top ) == "Top" );
}

// =============================================================================
// ELayoutType formatter
// =============================================================================

TEST_CASE( "ELayoutType formats all values", "[format][direction]" )
{
    REQUIRE( std::format( "{}", ELayoutType::Horizontal ) == "Horizontal" );
    REQUIRE( std::format( "{}", ELayoutType::Vertical   ) == "Vertical"   );
    REQUIRE( std::format( "{}", ELayoutType::Overlay    ) == "Overlay"    );
    REQUIRE( std::format( "{}", ELayoutType::Grid       ) == "Grid"       );
}

// =============================================================================
// Constraints formatter
// =============================================================================

TEST_CASE( "Constraints::Fixed formats with matching min and max", "[format][constraints]" )
{
    Constraints c = Constraints::Fixed( Vec2f( 100.0f, 200.0f ) );
    std::string s = std::format( "{}", c );
    REQUIRE( s.find( "Constraints" ) != std::string::npos );
    REQUIRE( s.find( "Min"         ) != std::string::npos );
    REQUIRE( s.find( "Max"         ) != std::string::npos );
    REQUIRE( s.find( "100"         ) != std::string::npos );
    REQUIRE( s.find( "200"         ) != std::string::npos );
}

TEST_CASE( "Constraints::Unbounded formats without crashing", "[format][constraints]" )
{
    Constraints c = Constraints::Unbounded();
    REQUIRE_NOTHROW( std::format( "{}", c ) );
}
