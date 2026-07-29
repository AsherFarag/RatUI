#pragma once

/**
 * @file TestLayoutCommon.h
 * @brief Shared helpers for exercising the RatUI layout engine (LayoutNode + Measure/Arrange)
 *        in isolation, without needing a Scene, widgets, or a rendering backend.
 */

#include "TestCommon.h"
#include <RatUI/Layout/LayoutEngine.h>
#include <vector>
#include <cmath>

/**
 * @brief Owns the scratch memory and LayoutContext needed to call MeasureLayoutNode/ArrangeLayoutNode.
 * Construct one per TEST_CASE; the buffer is generously sized for the small trees these tests build.
 */
struct LayoutFixture
{
    static constexpr u32 c_BufferSize = 64 * 1024;

    std::vector<u8> Buffer;
    BumpAllocator   Allocator;
    LayoutContext   Ctx;

    LayoutFixture()
        : Buffer( c_BufferSize )
        , Allocator( Buffer.data(), static_cast<u32>( Buffer.size() ) )
        , Ctx{ Allocator }
    {}
};

/**
 * @brief Runs one Measure + Arrange pass, allocating a_Root the full a_AvailableSize rect at the origin.
 * @note Widget-less trees (as used throughout these tests) never report HasWidthDependentContent,
 *       so a single pass is always sufficient here - no reflow loop is needed.
 */
inline void RunLayout( LayoutNode& a_Root, Vec2<Unit> a_AvailableSize, LayoutFixture& a_Fixture )
{
    MeasureLayoutNode( a_Root, a_AvailableSize, a_Fixture.Ctx );
    ArrangeLayoutNode( a_Root, Rect<Unit>{ Vec2<Unit>{ 0_u, 0_u }, a_AvailableSize }, a_Fixture.Ctx );
}

// Unit is a float under the hood, so exact equality is fragile - compare with a small absolute margin instead.

#define REQUIRE_UNIT( a_Actual, a_Expected ) \
    REQUIRE( ( a_Actual ).ToFloat() == Catch::Approx( a_Expected ).margin( 0.01 ) )

#define REQUIRE_VEC2( a_Actual, a_ExpectedX, a_ExpectedY )  \
    do {                                                    \
        REQUIRE_UNIT( ( a_Actual )[0], ( a_ExpectedX ) );   \
        REQUIRE_UNIT( ( a_Actual )[1], ( a_ExpectedY ) );   \
    } while ( false )

#define REQUIRE_RECT( a_Actual, a_X, a_Y, a_W, a_H )            \
    do {                                                        \
        REQUIRE_VEC2( ( a_Actual ).Origin, ( a_X ), ( a_Y ) );  \
        REQUIRE_VEC2( ( a_Actual ).Size,   ( a_W ), ( a_H ) );  \
    } while ( false )
