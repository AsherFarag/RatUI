/**
 * @file TestStringFormat.cpp
 * @brief Tests for RatUI std::formatter specialisations.
 */

#include "../Common/Common.h"
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

TEST_CASE( "EAlignment formats preset AlignTopLeft", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignTopLeft ) == "AlignTopLeft" );
}

TEST_CASE( "EAlignment formats preset AlignCenter", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignCenter ) == "AlignCenter" );
}

TEST_CASE( "EAlignment formats preset AlignBottomRight", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignBottomRight ) == "AlignBottomRight" );
}

TEST_CASE( "EAlignment formats all nine presets correctly", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignTopLeft     ) == "AlignTopLeft"     );
    REQUIRE( std::format( "{}", AlignTopCenter   ) == "AlignTopCenter"   );
    REQUIRE( std::format( "{}", AlignTopRight    ) == "AlignTopRight"    );
    REQUIRE( std::format( "{}", AlignCenterLeft  ) == "AlignCenterLeft"  );
    REQUIRE( std::format( "{}", AlignCenter      ) == "AlignCenter"      );
    REQUIRE( std::format( "{}", AlignCenterRight ) == "AlignCenterRight" );
    REQUIRE( std::format( "{}", AlignBottomLeft  ) == "AlignBottomLeft"  );
    REQUIRE( std::format( "{}", AlignBottomCenter) == "AlignBottomCenter");
    REQUIRE( std::format( "{}", AlignBottomRight ) == "AlignBottomRight" );
}

TEST_CASE( "EAlignment zero value formats as None", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", static_cast<EAlignment>( 0 ) ) == "None" );
}

TEST_CASE( "EAlignment formats individual flag AlignLeft", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignLeft ) == "AlignLeft" );
}

TEST_CASE( "EAlignment formats individual flag AlignTop", "[format][alignment]" )
{
    REQUIRE( std::format( "{}", AlignTop ) == "AlignTop" );
}

// =============================================================================
// ELayoutDirection formatter
// =============================================================================

TEST_CASE( "ELayoutDirection formats all values", "[format][direction]" )
{
    REQUIRE( std::format( "{}", ELayoutDirection::Horizontal ) == "Horizontal" );
    REQUIRE( std::format( "{}", ELayoutDirection::Vertical   ) == "Vertical"   );
    REQUIRE( std::format( "{}", ELayoutDirection::Stack      ) == "Stack"      );
    REQUIRE( std::format( "{}", ELayoutDirection::Grid       ) == "Grid"       );
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
