/**
 * @file TestContainers.cpp
 * @brief Tests for RatUI container type aliases and free-function wrappers.
 */

#include "TestCommon.h"

// =============================================================================
// Array (std::vector by default)
// =============================================================================

TEST_CASE( "Array is default constructible and empty", "[containers][array]" )
{
    Array<int> arr;
    REQUIRE( Empty( arr ) );
    REQUIRE( Size( arr ) == 0 );
}

TEST_CASE( "PushBack adds elements to Array", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 1 );
    PushBack( arr, 2 );
    PushBack( arr, 3 );
    REQUIRE( Size( arr ) == 3 );
}

TEST_CASE( "EmplaceBack adds elements to Array", "[containers][array]" )
{
    Array<int> arr;
    EmplaceBack( arr, 10 );
    EmplaceBack( arr, 20 );
    REQUIRE( Size( arr ) == 2 );
}

TEST_CASE( "Front and Back return correct elements", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 42 );
    PushBack( arr, 99 );
    REQUIRE( Front( arr ) == 42 );
    REQUIRE( Back( arr )  == 99 );
}

TEST_CASE( "At accesses element by index", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 10 );
    PushBack( arr, 20 );
    PushBack( arr, 30 );
    REQUIRE( At( arr, 0 ) == 10 );
    REQUIRE( At( arr, 1 ) == 20 );
    REQUIRE( At( arr, 2 ) == 30 );
}

TEST_CASE( "At allows element modification", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 10 );
    At( arr, 0 ) = 99;
    REQUIRE( At( arr, 0 ) == 99 );
}

TEST_CASE( "Data returns pointer to first element", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 7 );
    REQUIRE( Data( arr ) == &At( arr, 0 ) );
}

TEST_CASE( "Clear empties the Array", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 1 );
    PushBack( arr, 2 );
    Clear( arr );
    REQUIRE( Empty( arr ) );
}

TEST_CASE( "Reserve does not change size", "[containers][array]" )
{
    Array<int> arr;
    Reserve( arr, 100 );
    REQUIRE( Size( arr ) == 0 );
}

TEST_CASE( "Resize changes size", "[containers][array]" )
{
    Array<int> arr;
    Resize( arr, 5 );
    REQUIRE( Size( arr ) == 5 );
}

TEST_CASE( "Erase removes an element", "[containers][array]" )
{
    Array<int> arr;
    PushBack( arr, 1 );
    PushBack( arr, 2 );
    PushBack( arr, 3 );
    Erase( arr, Begin( arr ) );
    REQUIRE( Size( arr ) == 2 );
    REQUIRE( At( arr, 0 ) == 2 );
}

// =============================================================================
// FixedArray (std::array by default)
// =============================================================================

TEST_CASE( "FixedArray has correct size", "[containers][fixed-array]" )
{
    FixedArray<int, 4> arr = { 1, 2, 3, 4 };
    REQUIRE( Size( arr ) == 4 );
}

TEST_CASE( "FixedArray element access via At", "[containers][fixed-array]" )
{
    FixedArray<float, 3> arr = { 1.0f, 2.0f, 3.0f };
    REQUIRE( At( arr, 0 ) == 1.0f );
    REQUIRE( At( arr, 1 ) == 2.0f );
    REQUIRE( At( arr, 2 ) == 3.0f );
}

TEST_CASE( "FixedArray Front and Back", "[containers][fixed-array]" )
{
    FixedArray<int, 3> arr = { 10, 20, 30 };
    REQUIRE( Front( arr ) == 10 );
    REQUIRE( Back( arr )  == 30 );
}

TEST_CASE( "FixedArray is not empty", "[containers][fixed-array]" )
{
    FixedArray<int, 2> arr = { 0, 0 };
    REQUIRE_FALSE( Empty( arr ) );
}

// =============================================================================
// Span (std::span by default)
// =============================================================================

TEST_CASE( "Span over Array gives correct size and elements", "[containers][span]" )
{
    Array<int> arr;
    PushBack( arr, 10 );
    PushBack( arr, 20 );
    PushBack( arr, 30 );

    Span<int> sp( arr );
    REQUIRE( Size( sp ) == 3 );
    REQUIRE( RawAt( sp, 0 ) == 10 );
    REQUIRE( RawAt( sp, 1 ) == 20 );
    REQUIRE( RawAt( sp, 2 ) == 30 );
}

TEST_CASE( "Span Begin and End iterate over all elements", "[containers][span]" )
{
    Array<int> arr;
    PushBack( arr, 1 );
    PushBack( arr, 2 );
    PushBack( arr, 3 );

    Span<int> sp( arr );
    int sum = 0;
    for ( auto it = Begin( sp ); it != End( sp ); ++it )
        sum += *it;

    REQUIRE( sum == 6 );
}