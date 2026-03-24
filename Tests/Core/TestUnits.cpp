/**
 * @file TestUnits.cpp
 * @brief Tests for RatUI unit types: Pixel, Radians, Degrees, and their literals.
 */

#include "../Common/Common.h"
#include <cmath>

using namespace RatUI;

// =============================================================================
// Pixel
// =============================================================================

TEST_CASE( "Pixel default construction is zero", "[units][pixel]" )
{
    Pixel p{};
    REQUIRE( p.Value == 0u );
}

TEST_CASE( "Pixel explicit construction stores value", "[units][pixel]" )
{
    Pixel p{ 42u };
    REQUIRE( p.Value == 42u );
}

TEST_CASE( "Pixel equality operator", "[units][pixel]" )
{
    Pixel a{ 10u };
    Pixel b{ 10u };
    Pixel c{ 20u };
    REQUIRE( a == b );
    REQUIRE( a != c );
}

TEST_CASE( "Pixel comparison operators", "[units][pixel]" )
{
    Pixel small{ 5u };
    Pixel large{ 10u };
    REQUIRE( small < large );
    REQUIRE( large > small );
    REQUIRE( small <= small );
    REQUIRE( large >= large );
}

TEST_CASE( "Pixel addition", "[units][pixel]" )
{
    Pixel a{ 10u };
    Pixel b{ 5u };
    REQUIRE( ( a + b ).Value == 15u );
}

TEST_CASE( "Pixel subtraction", "[units][pixel]" )
{
    Pixel a{ 10u };
    Pixel b{ 3u };
    REQUIRE( ( a - b ).Value == 7u );
}

TEST_CASE( "Pixel scalar multiplication", "[units][pixel]" )
{
    Pixel p{ 10u };
    REQUIRE( ( p * 2.0f ).Value == 20u );
}

TEST_CASE( "Pixel left-hand scalar multiplication", "[units][pixel]" )
{
    Pixel p{ 10u };
    REQUIRE( ( 3.0f * p ).Value == 30u );
}

TEST_CASE( "Pixel scalar division", "[units][pixel]" )
{
    Pixel p{ 20u };
    REQUIRE( ( p / 4.0f ).Value == 5u );
}

TEST_CASE( "Pixel addition-assignment", "[units][pixel]" )
{
    Pixel p{ 10u };
    p += Pixel{ 5u };
    REQUIRE( p.Value == 15u );
}

TEST_CASE( "Pixel subtraction-assignment", "[units][pixel]" )
{
    Pixel p{ 10u };
    p -= Pixel{ 3u };
    REQUIRE( p.Value == 7u );
}

TEST_CASE( "Pixel multiplication-assignment", "[units][pixel]" )
{
    Pixel p{ 10u };
    p *= 2.0f;
    REQUIRE( p.Value == 20u );
}

TEST_CASE( "Pixel division-assignment", "[units][pixel]" )
{
    Pixel p{ 20u };
    p /= 4.0f;
    REQUIRE( p.Value == 5u );
}

// =============================================================================
// Radians
// =============================================================================

TEST_CASE( "Radians default construction is zero", "[units][radians]" )
{
    Radians<f32> r{};
    REQUIRE( r.Value == 0.0f );
}

TEST_CASE( "Radians explicit construction stores value", "[units][radians]" )
{
    Radians<f32> r{ 1.5f };
    REQUIRE( r.Value == 1.5f );
}

TEST_CASE( "Radians equality operator", "[units][radians]" )
{
    Radians<f32> a{ 1.0f };
    Radians<f32> b{ 1.0f };
    Radians<f32> c{ 2.0f };
    REQUIRE( a == b );
    REQUIRE( a != c );
}

TEST_CASE( "Radians addition", "[units][radians]" )
{
    Radians<f32> a{ 1.0f };
    Radians<f32> b{ 0.5f };
    REQUIRE( ( a + b ).Value == Catch::Approx( 1.5f ) );
}

TEST_CASE( "Radians subtraction", "[units][radians]" )
{
    Radians<f32> a{ 2.0f };
    Radians<f32> b{ 0.5f };
    REQUIRE( ( a - b ).Value == Catch::Approx( 1.5f ) );
}

TEST_CASE( "Radians scalar multiplication", "[units][radians]" )
{
    Radians<f32> r{ 1.0f };
    REQUIRE( ( r * 2.0f ).Value == Catch::Approx( 2.0f ) );
}

TEST_CASE( "Radians scalar division", "[units][radians]" )
{
    Radians<f32> r{ 2.0f };
    REQUIRE( ( r / 2.0f ).Value == Catch::Approx( 1.0f ) );
}

TEST_CASE( "Radians negation", "[units][radians]" )
{
    Radians<f32> r{ 1.0f };
    REQUIRE( ( -r ).Value == Catch::Approx( -1.0f ) );
}

TEST_CASE( "Radians converts to Degrees", "[units][radians]" )
{
    Radians<f32> r{ Pi<f32> };
    Degrees<f32> d{ r };
    REQUIRE( d.Value == Catch::Approx( 180.0f ).epsilon( k_FloatEpsilon ) );
}

TEST_CASE( "Radians constructs from Degrees", "[units][radians]" )
{
    Degrees<f32> d{ 90.0f };
    Radians<f32> r{ d };
    REQUIRE( r.Value == Catch::Approx( Pi<f32> / 2.0f ).epsilon( k_FloatEpsilon ) );
}

// =============================================================================
// Degrees
// =============================================================================

TEST_CASE( "Degrees default construction is zero", "[units][degrees]" )
{
    Degrees<f32> d{};
    REQUIRE( d.Value == 0.0f );
}

TEST_CASE( "Degrees explicit construction stores value", "[units][degrees]" )
{
    Degrees<f32> d{ 45.0f };
    REQUIRE( d.Value == 45.0f );
}

TEST_CASE( "Degrees equality operator", "[units][degrees]" )
{
    Degrees<f32> a{ 90.0f };
    Degrees<f32> b{ 90.0f };
    Degrees<f32> c{ 45.0f };
    REQUIRE( a == b );
    REQUIRE( a != c );
}

TEST_CASE( "Degrees addition", "[units][degrees]" )
{
    Degrees<f32> a{ 90.0f };
    Degrees<f32> b{ 45.0f };
    REQUIRE( ( a + b ).Value == Catch::Approx( 135.0f ) );
}

TEST_CASE( "Degrees subtraction", "[units][degrees]" )
{
    Degrees<f32> a{ 180.0f };
    Degrees<f32> b{ 90.0f };
    REQUIRE( ( a - b ).Value == Catch::Approx( 90.0f ) );
}

TEST_CASE( "Degrees scalar multiplication", "[units][degrees]" )
{
    Degrees<f32> d{ 45.0f };
    REQUIRE( ( d * 2.0f ).Value == Catch::Approx( 90.0f ) );
}

TEST_CASE( "Degrees scalar division", "[units][degrees]" )
{
    Degrees<f32> d{ 90.0f };
    REQUIRE( ( d / 2.0f ).Value == Catch::Approx( 45.0f ) );
}

TEST_CASE( "Degrees negation", "[units][degrees]" )
{
    Degrees<f32> d{ 90.0f };
    REQUIRE( ( -d ).Value == Catch::Approx( -90.0f ) );
}

TEST_CASE( "Degrees converts to Radians", "[units][degrees]" )
{
    Degrees<f32> d{ 180.0f };
    Radians<f32> r{ d };
    REQUIRE( r.Value == Catch::Approx( Pi<f32> ).epsilon( k_FloatEpsilon ) );
}

TEST_CASE( "Degrees constructs from Radians", "[units][degrees]" )
{
    Radians<f32> r{ Pi<f32> / 2.0f };
    Degrees<f32> d{ r };
    REQUIRE( d.Value == Catch::Approx( 90.0f ).epsilon( k_FloatEpsilon ) );
}

// =============================================================================
// Literals
// =============================================================================

TEST_CASE( "Pixel literal _px creates correct value", "[units][literals]" )
{
    Pixel p = 16_px;
    REQUIRE( p.Value == 16u );
}

TEST_CASE( "Radians literal _rad creates correct value", "[units][literals]" )
{
    Radians<f64> r = 1.0_rad;
    REQUIRE( r.Value == Catch::Approx( 1.0 ) );
}

TEST_CASE( "Degrees literal _deg creates correct value", "[units][literals]" )
{
    Degrees<f64> d = 90.0_deg;
    REQUIRE( d.Value == Catch::Approx( 90.0 ) );
}

TEST_CASE( "Integer _rad literal creates Radians", "[units][literals]" )
{
    Radians<f64> r = 1_rad;
    REQUIRE( r.Value == Catch::Approx( 1.0 ) );
}

TEST_CASE( "Integer _deg literal creates Degrees", "[units][literals]" )
{
    Degrees<f64> d = 180_deg;
    REQUIRE( d.Value == Catch::Approx( 180.0 ) );
}
