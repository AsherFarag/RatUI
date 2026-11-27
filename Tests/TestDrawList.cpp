/**
 * @file TestDrawList.cpp
 * @brief DrawList state that needs no renderer output: clips and transforms.
 */

#include "TestCommon.h"

TEST_CASE( "DrawList: a clip pushed under a transform moves with it", "[DrawList]" )
{
    Null::NullRenderer    renderer;
    Null::NullTextMetrics metrics;
    GlyphAtlas            atlas{ renderer, metrics };
    DrawList              drawer{ atlas };

    drawer.PushClipRect( { { 0_u, 0_u }, { 200_u, 100_u } } );
    drawer.PushTransform( Mat3<Unit>::from_columns( Vec3<Unit>{ 1_u, 0_u, 0_u }, Vec3<Unit>{ 0_u, 1_u, 0_u }, Vec3<Unit>{ 0_u, -90_u, 1_u } ) );

    // Content y 100..130 is drawn at 10..40.
    drawer.PushClipRect( { { 0_u, 100_u }, { 100_u, 30_u } } );
    const Rect<Unit> clip = *drawer.GetCurrentClip();
    REQUIRE( clip.Top().ToFloat() == Catch::Approx( 10.f ) );
    REQUIRE( clip.Height().ToFloat() == Catch::Approx( 30.f ) );

    drawer.PopClipRect();
    drawer.PopTransform();
    drawer.PopClipRect();
}
