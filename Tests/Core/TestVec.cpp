/**
 * @file TestVec.cpp
 * @brief Tests for RatUI::Detail::Vec and RatUI::Rectf.
 */

#include "../Common/Common.h"

using namespace RatUI;

// =============================================================================
// Vec construction
// =============================================================================

TEST_CASE( "Vec2f default construction initialises to zero", "[math][vec]" )
{
    Vec2f v;
    REQUIRE( v[ 0 ] == 0.0f );
    REQUIRE( v[ 1 ] == 0.0f );
}

TEST_CASE( "Vec2f single-value construction broadcasts to all components", "[math][vec]" )
{
    Vec2f v( 5.0f );
    REQUIRE( v[ 0 ] == 5.0f );
    REQUIRE( v[ 1 ] == 5.0f );
}

TEST_CASE( "Vec2f element-wise construction", "[math][vec]" )
{
    Vec2f v( 1.0f, 2.0f );
    REQUIRE( v[ 0 ] == 1.0f );
    REQUIRE( v[ 1 ] == 2.0f );
}

TEST_CASE( "Vec3f element-wise construction", "[math][vec]" )
{
    Vec3f v( 1.0f, 2.0f, 3.0f );
    REQUIRE( v[ 0 ] == 1.0f );
    REQUIRE( v[ 1 ] == 2.0f );
    REQUIRE( v[ 2 ] == 3.0f );
}

TEST_CASE( "Vec2f copy construction", "[math][vec]" )
{
    Vec2f a( 3.0f, 4.0f );
    Vec2f b = a;
    REQUIRE( b[ 0 ] == 3.0f );
    REQUIRE( b[ 1 ] == 4.0f );
}

// =============================================================================
// Vec arithmetic operators
// =============================================================================

TEST_CASE( "Vec2f addition", "[math][vec]" )
{
    Vec2f a( 1.0f, 2.0f );
    Vec2f b( 3.0f, 4.0f );
    Vec2f c = a + b;
    REQUIRE( c[ 0 ] == 4.0f );
    REQUIRE( c[ 1 ] == 6.0f );
}

//TEST_CASE( "Vec2f addition-assignment", "[math][vec]" )
//{
//    Vec2f a( 1.0f, 2.0f );
//    a += Vec2f( 3.0f, 4.0f );
//    REQUIRE( a[ 0 ] == 4.0f );
//    REQUIRE( a[ 1 ] == 6.0f );
//}

TEST_CASE( "Vec2f subtraction", "[math][vec]" )
{
    Vec2f a( 5.0f, 7.0f );
    Vec2f b( 3.0f, 2.0f );
    Vec2f c = a - b;
    REQUIRE( c[ 0 ] == 2.0f );
    REQUIRE( c[ 1 ] == 5.0f );
}

//TEST_CASE( "Vec2f subtraction-assignment", "[math][vec]" )
//{
//    Vec2f a( 5.0f, 7.0f );
//    a -= Vec2f( 3.0f, 2.0f );
//    REQUIRE( a[ 0 ] == 2.0f );
//    REQUIRE( a[ 1 ] == 5.0f );
//}

TEST_CASE( "Vec2f scalar multiplication", "[math][vec]" )
{
    Vec2f a( 2.0f, 3.0f );
    Vec2f b = a * 4.0f;
    REQUIRE( b[ 0 ] == 8.0f );
    REQUIRE( b[ 1 ] == 12.0f );
}

TEST_CASE( "Vec2f scalar multiplication left-hand side", "[math][vec]" )
{
    Vec2f a( 2.0f, 3.0f );
    Vec2f b = 4.0f * a;
    REQUIRE( b[ 0 ] == 8.0f );
    REQUIRE( b[ 1 ] == 12.0f );
}

TEST_CASE( "Vec2f scalar multiplication-assignment", "[math][vec]" )
{
    Vec2f a( 2.0f, 3.0f );
    a *= 4.0f;
    REQUIRE( a[ 0 ] == 8.0f );
    REQUIRE( a[ 1 ] == 12.0f );
}

TEST_CASE( "Vec2f scalar division", "[math][vec]" )
{
    Vec2f a( 8.0f, 12.0f );
    Vec2f b = a / 4.0f;
    REQUIRE( b[ 0 ] == Catch::Approx( 2.0f ) );
    REQUIRE( b[ 1 ] == Catch::Approx( 3.0f ) );
}

TEST_CASE( "Vec2f scalar division-assignment", "[math][vec]" )
{
    Vec2f a( 8.0f, 12.0f );
    a /= 4.0f;
    REQUIRE( a[ 0 ] == Catch::Approx( 2.0f ) );
    REQUIRE( a[ 1 ] == Catch::Approx( 3.0f ) );
}

// =============================================================================
// Vec comparison
// =============================================================================

TEST_CASE( "Vec2f equality", "[math][vec]" )
{
    Vec2f a( 1.0f, 2.0f );
    Vec2f b( 1.0f, 2.0f );
    Vec2f c( 1.0f, 3.0f );
    REQUIRE( a == b );
    REQUIRE( a != c );
}

// =============================================================================
// Vec math operations
// =============================================================================

//TEST_CASE( "Vec2f dot product", "[math][vec]" )
//{
//    Vec2f a( 1.0f, 2.0f );
//    Vec2f b( 3.0f, 4.0f );
//    REQUIRE( a.Dot( b ) == Catch::Approx( 11.0f ) ); // 1*3 + 2*4
//}
//
//TEST_CASE( "Vec2f length squared", "[math][vec]" )
//{
//    Vec2f v( 3.0f, 4.0f );
//    REQUIRE( v.LengthSq() == Catch::Approx( 25.0f ) );
//}
//
//TEST_CASE( "Vec2f length", "[math][vec]" )
//{
//    Vec2f v( 3.0f, 4.0f );
//    REQUIRE( v.Length() == Catch::Approx( 5.0f ) );
//}
//
//TEST_CASE( "Vec2f normalise produces unit vector", "[math][vec]" )
//{
//    Vec2f v( 3.0f, 4.0f );
//    Vec2f n = v.Normalized();
//    REQUIRE( n.Length() == Catch::Approx( 1.0f ).epsilon( k_FloatEpsilon ) );
//    REQUIRE( n[ 0 ] == Catch::Approx( 0.6f ).epsilon( k_FloatEpsilon ) );
//    REQUIRE( n[ 1 ] == Catch::Approx( 0.8f ).epsilon( k_FloatEpsilon ) );
//}
//
//TEST_CASE( "Vec2f normalise of zero vector returns zero", "[math][vec]" )
//{
//    Vec2f v;
//    Vec2f n = v.Normalized();
//    REQUIRE( n[ 0 ] == 0.0f );
//    REQUIRE( n[ 1 ] == 0.0f );
//}

// =============================================================================
// Rectf construction
// =============================================================================

TEST_CASE( "Rectf default construction", "[math][rect]" )
{
    Rectf r;
    REQUIRE( r.Origin[ 0 ] == 0.0f );
    REQUIRE( r.Origin[ 1 ] == 0.0f );
    REQUIRE( r.Size[ 0 ] == 0.0f );
    REQUIRE( r.Size[ 1 ] == 0.0f );
}

TEST_CASE( "Rectf FromMinMax produces correct bounds", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 10.0f, 20.0f ), Vec2f( 110.0f, 220.0f ) );
    REQUIRE( r.Left()   == Catch::Approx( 10.0f ) );
    REQUIRE( r.Right()  == Catch::Approx( 110.0f ) );
    REQUIRE( r.Top()    == Catch::Approx( 20.0f ) );
    REQUIRE( r.Bottom() == Catch::Approx( 220.0f ) );
}

TEST_CASE( "Rectf corner accessors", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 100.0f, 50.0f ) );
    RequireApproxEqual( r.TopLeft(),     Vec2f( 0.0f,   0.0f  ) );
    RequireApproxEqual( r.TopRight(),    Vec2f( 100.0f, 0.0f  ) );
    RequireApproxEqual( r.BottomLeft(),  Vec2f( 0.0f,   50.0f ) );
    RequireApproxEqual( r.BottomRight(), Vec2f( 100.0f, 50.0f ) );
}

TEST_CASE( "Rectf Min and Max match TopLeft and BottomRight", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 5.0f, 10.0f ), Vec2f( 50.0f, 80.0f ) );
    RequireApproxEqual( r.Min(), r.TopLeft()     );
    RequireApproxEqual( r.Max(), r.BottomRight() );
}

TEST_CASE( "Rectf Size returns correct dimensions", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 200.0f, 100.0f ) );
    RequireApproxEqual( r.Size, Vec2f( 200.0f, 100.0f ) );
}

// =============================================================================
// Rectf geometry tests
// =============================================================================

TEST_CASE( "Rectf Contains point inside", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 100.0f, 100.0f ) );
    REQUIRE( r.Contains( Vec2f( 50.0f, 50.0f ) ) );
}

TEST_CASE( "Rectf Contains point on border", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 100.0f, 100.0f ) );
    REQUIRE( r.Contains( Vec2f( 0.0f,   0.0f   ) ) );
    REQUIRE( r.Contains( Vec2f( 100.0f, 100.0f ) ) );
}

TEST_CASE( "Rectf Contains point outside", "[math][rect]" )
{
    Rectf r = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 100.0f, 100.0f ) );
    REQUIRE_FALSE( r.Contains( Vec2f( -1.0f,  50.0f  ) ) );
    REQUIRE_FALSE( r.Contains( Vec2f( 101.0f, 50.0f  ) ) );
    REQUIRE_FALSE( r.Contains( Vec2f( 50.0f,  -1.0f  ) ) );
    REQUIRE_FALSE( r.Contains( Vec2f( 50.0f,  101.0f ) ) );
}

TEST_CASE( "Rectf Intersects overlapping rects", "[math][rect]" )
{
    Rectf a = Rectf::FromMinMax( Vec2f( 0.0f,  0.0f  ), Vec2f( 100.0f, 100.0f ) );
    Rectf b = Rectf::FromMinMax( Vec2f( 50.0f, 50.0f ), Vec2f( 150.0f, 150.0f ) );
    REQUIRE( a.Intersects( b ) );
    REQUIRE( b.Intersects( a ) );
}

TEST_CASE( "Rectf Intersects non-overlapping rects returns false", "[math][rect]" )
{
    Rectf a = Rectf::FromMinMax( Vec2f( 0.0f,   0.0f   ), Vec2f( 100.0f, 100.0f ) );
    Rectf b = Rectf::FromMinMax( Vec2f( 200.0f, 200.0f ), Vec2f( 300.0f, 300.0f ) );
    REQUIRE_FALSE( a.Intersects( b ) );
}

TEST_CASE( "Rectf Intersection of overlapping rects", "[math][rect]" )
{
    Rectf a = Rectf::FromMinMax( Vec2f( 0.0f,  0.0f  ), Vec2f( 100.0f, 100.0f ) );
    Rectf b = Rectf::FromMinMax( Vec2f( 50.0f, 50.0f ), Vec2f( 150.0f, 150.0f ) );
    Rectf i = a.Intersection( b );
    RequireApproxEqual( i.Min(), Vec2f( 50.0f,  50.0f  ) );
    RequireApproxEqual( i.Max(), Vec2f( 100.0f, 100.0f ) );
}

TEST_CASE( "Rectf Intersection of non-overlapping rects is empty", "[math][rect]" )
{
    Rectf a = Rectf::FromMinMax( Vec2f( 0.0f,   0.0f   ), Vec2f( 100.0f, 100.0f ) );
    Rectf b = Rectf::FromMinMax( Vec2f( 200.0f, 200.0f ), Vec2f( 300.0f, 300.0f ) );
    Rectf i = a.Intersection( b );
    // An empty intersection has Max < Min after clamping, resulting in zero rect.
    REQUIRE( i.Size[ 0 ] == 0.0f );
    REQUIRE( i.Size[ 1 ] == 0.0f );
}
