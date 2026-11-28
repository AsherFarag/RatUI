/**
 * @file TestLayoutEngine.cpp
 * @brief Tests for MeasureLayoutNode / ArrangeLayoutNode: sizing modes, padding/margin/spacing,
 *        flex distribution, alignment, overlay stacking, anchored positioning, grid layout,
 *        how EVisibility interacts with layout, and Measure's dirty-based memoization.
 *
 * All trees here are built from plain, widget-less LayoutNodes (a_Node.Widget == nullptr), so
 * content sizing comes purely from children/padding, never from IWidget::OnMeasureContent, and
 * ArrangeLayoutNode's reflow path (WidgetTraits::WidthDependentContent) never triggers.
 */

#include "TestLayoutCommon.h"
#include <RatUI/Widget/IWidget.h>

// =============================================================================
// Sizing modes
// =============================================================================

TEST_CASE( "Fixed sizing ignores the available size entirely", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.FixedWidth( 120_u ).FixedHeight( 40_u );

    Vec2<Unit> desired = MeasureLayoutNode( node, Vec2<Unit>{ 10_u, 10_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 120.f, 40.f );
}

TEST_CASE( "Percent sizing resolves against the available size passed into the node", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.PercentWidth( 0.5f ).PercentHeight( 0.25f );

    Vec2<Unit> desired = MeasureLayoutNode( node, Vec2<Unit>{ 400_u, 200_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 200.f, 50.f );
}

TEST_CASE( "Content sizing with no widget and no children collapses to zero", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode node; // WidthMode/HeightMode default to Content

    Vec2<Unit> desired = MeasureLayoutNode( node, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 0.f, 0.f );
}

TEST_CASE( "Content sizing aggregates a single child plus padding", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.Padding( Edges::All( 10_u ) ); // LayoutType defaults to Overlay
    child.FixedWidth( 50_u ).FixedHeight( 30_u );

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 70.f, 50.f ); // 50+2*10, 30+2*10
}

TEST_CASE( "SizeConstraints clamp the final desired size regardless of sizing mode", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.FixedWidth( 500_u ).FixedHeight( 500_u )
        .SizeConstraints( Constraints::AtMost( Vec2<Unit>{ 100_u, 100_u } ) );

    Vec2<Unit> desired = MeasureLayoutNode( node, Vec2<Unit>{ 1000_u, 1000_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 100.f, 100.f );
}

TEST_CASE( "A percent-width child contributes zero to its content-sized parent's auto width", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    child.PercentWidth( 0.5f ).FixedHeight( 20_u );

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    // Percent-sized children are excluded from that axis's contribution to an auto-sized
    // parent, to avoid a circular "my size depends on your size which depends on my size".
    REQUIRE_UNIT( desired[0], 0.f );
    REQUIRE_UNIT( desired[1], 20.f );
}

// =============================================================================
// Padding / Margin / Spacing
// =============================================================================

TEST_CASE( "Padding insets the space available to children and offsets their arranged position", "[layout-engine][padding]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 100_u ).Padding( Edges::All( 20_u ) );
    child.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 20.f, 20.f, 160.f, 60.f );
}

TEST_CASE( "Margin insets a child within the space its parent allocated to it", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u );
    child.FlexWidth().FlexHeight().Margin( Edges::All( 10_u ) );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 10.f, 10.f, 80.f, 80.f );
}

TEST_CASE( "Margin is included when a content-sized parent measures a fixed-size child", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    child.FixedWidth( 50_u ).FixedHeight( 50_u ).Margin( Edges::All( 5_u ) );

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 60.f, 60.f ); // 50 + 2*5
}

TEST_CASE( "Margin sits around a fixed-size child in a linear layout, without shrinking it", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.LayoutType( ELayoutType::Vertical );
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    a.FixedWidth( 20_u ).FixedHeight( 20_u ).Margin( Edges{ .T = 5_u, .B = 10_u, .L = 3_u } );
    b.FixedWidth( 20_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_RECT( a.Layout.ParentRect, 3.f, 5.f, 20.f, 20.f );
    REQUIRE_RECT( b.Layout.ParentRect, 0.f, 35.f, 20.f, 20.f ); // After a's slot: 5 + 20 + 10.
}

TEST_CASE( "Margin is part of the slot that cross-axis alignment places", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::BottomLeft );
    parent.PushBackChild( child );
    child.FixedWidth( 20_u ).FixedHeight( 20_u ).Margin( Edges{ .B = 10_u } );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );
    REQUIRE_RECT( child.Layout.ParentRect, 0.f, 70.f, 20.f, 20.f );

    // Stretched across, the child fills what its margin leaves.
    child.SelfAlign( EAlign::VStretch );
    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );
    REQUIRE_RECT( child.Layout.ParentRect, 0.f, 0.f, 20.f, 90.f );
}

TEST_CASE( "Margin sits around a fixed-size child in a grid cell, and stretching fills the cell less the margin", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.LayoutType( ELayoutType::Grid ).GridColumns( 2 ).GridRows( 0 );
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    a.FixedWidth( 20_u ).FixedHeight( 20_u ).Margin( Edges::All( 5_u ) );
    b.FixedWidth( 10_u ).FixedHeight( 10_u ).Margin( Edges{ .L = 2_u } ).SelfAlign( EAlign::StretchFill );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_RECT( a.Layout.ParentRect, 5.f, 5.f, 20.f, 20.f );
    REQUIRE_RECT( b.Layout.ParentRect, 32.f, 0.f, 10.f, 30.f ); // The 12 x 30 cell after a's 30 x 30 one, less 2 on the left.
}

TEST_CASE( "Spacing separates children in a Horizontal layout but is not added after the last one", "[layout-engine][spacing]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );
    parent.LayoutType( ELayoutType::Horizontal ).Spacing( 10_u );
    a.FixedWidth( 20_u ).FixedHeight( 20_u );
    b.FixedWidth( 20_u ).FixedHeight( 20_u );
    c.FixedWidth( 20_u ).FixedHeight( 20_u );

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    REQUIRE_UNIT( desired[0], 80.f ); // 3*20 + 2 gaps of 10 (not 3 gaps)
}

TEST_CASE( "Horizontal layout positions children left to right, advancing by Spacing", "[layout-engine][arrange][horizontal]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.LayoutType( ELayoutType::Horizontal ).Spacing( 10_u )
          .FixedWidth( 400_u ).FixedHeight( 100_u );
    a.FixedWidth( 50_u ).FixedHeight( 50_u );
    b.FixedWidth( 60_u ).FixedHeight( 60_u );

    RunLayout( parent, Vec2<Unit>{ 400_u, 100_u }, fx );

    REQUIRE_RECT( a.Layout.SceneRect,  0.f, 0.f, 50.f, 50.f );
    REQUIRE_RECT( b.Layout.SceneRect, 60.f, 0.f, 60.f, 60.f ); // 50 + 10 spacing
}

TEST_CASE( "Vertical layout positions children top to bottom, advancing by Spacing", "[layout-engine][arrange][vertical]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.LayoutType( ELayoutType::Vertical ).Spacing( 5_u )
          .FixedWidth( 100_u ).FixedHeight( 400_u );
    a.FixedWidth( 30_u ).FixedHeight( 40_u );
    b.FixedWidth( 30_u ).FixedHeight( 50_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 400_u }, fx );

    REQUIRE_RECT( a.Layout.SceneRect, 0.f,  0.f, 30.f, 40.f );
    REQUIRE_RECT( b.Layout.SceneRect, 0.f, 45.f, 30.f, 50.f ); // 40 + 5 spacing
}

// =============================================================================
// Flex distribution
// =============================================================================

TEST_CASE( "FlexGrow distributes leftover main-axis space proportionally", "[layout-engine][flex]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 400_u ).FixedHeight( 100_u );
    a.FlexWidth().FlexHeight().FlexGrow( 1.f );
    b.FlexWidth().FlexHeight().FlexGrow( 1.f );
    c.FlexWidth().FlexHeight().FlexGrow( 2.f );

    RunLayout( parent, Vec2<Unit>{ 400_u, 100_u }, fx );

    // Total grow weight = 4: a and b each get 1/4 (100), c gets 2/4 (200).
    REQUIRE_RECT( a.Layout.SceneRect,   0.f, 0.f, 100.f, 100.f );
    REQUIRE_RECT( b.Layout.SceneRect, 100.f, 0.f, 100.f, 100.f );
    REQUIRE_RECT( c.Layout.SceneRect, 200.f, 0.f, 200.f, 100.f );
}

TEST_CASE( "Flex children without an explicit FlexGrow share the remaining space equally", "[layout-engine][flex]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 200_u ).FixedHeight( 50_u );
    a.FlexWidth().FlexHeight();
    b.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 200_u, 50_u }, fx );

    REQUIRE_UNIT( a.Layout.SceneRect.Size[0], 100.f );
    REQUIRE_UNIT( b.Layout.SceneRect.Size[0], 100.f );
}

TEST_CASE( "Fixed siblings reserve their space before flex children divide the remainder", "[layout-engine][flex]" )
{
    LayoutFixture fx;
    LayoutNode parent, fixed, flex;
    parent.PushBackChild( fixed );
    parent.PushBackChild( flex );
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 300_u ).FixedHeight( 50_u );
    fixed.FixedWidth( 100_u ).FixedHeight( 50_u );
    flex.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 300_u, 50_u }, fx );

    REQUIRE_UNIT( flex.Layout.SceneRect.Size[0], 200.f ); // 300 - 100
}

TEST_CASE( "A flex child's SizeConstraints clamp its own share without redistributing the remainder to siblings", "[layout-engine][flex]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 400_u ).FixedHeight( 50_u );
    a.FlexWidth().FlexHeight().SizeConstraints( Constraints::AtMost( Vec2<Unit>{ 80_u, Limits<Unit>::max() } ) );
    b.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 400_u, 50_u }, fx );

    // Each child's equal 200-unit share is computed independently from the same pool;
    // clamping `a` down to 80 does not hand the freed 120 units to `b`, and 120 units
    // of the container are simply left unused. This is a single-pass distribution,
    // not an iterative flexbox-style solver.
    REQUIRE_RECT( a.Layout.SceneRect,  0.f, 0.f,  80.f, 50.f );
    REQUIRE_RECT( b.Layout.SceneRect, 80.f, 0.f, 200.f, 50.f );
}

// =============================================================================
// Alignment (linear layouts)
// =============================================================================

TEST_CASE( "ChildAlign centers a child on the cross axis of a Horizontal layout", "[layout-engine][alignment]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.LayoutType( ELayoutType::Horizontal ).ChildAlign( EAlign::VCenter )
          .FixedWidth( 200_u ).FixedHeight( 100_u );
    child.FixedWidth( 20_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    REQUIRE_UNIT( child.Layout.SceneRect.Origin[1], 40.f ); // (100-20)/2
}

TEST_CASE( "SelfAlign overrides the parent's ChildAlign for a single child", "[layout-engine][alignment]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.LayoutType( ELayoutType::Horizontal ).ChildAlign( EAlign::Top )
          .FixedWidth( 200_u ).FixedHeight( 100_u );
    a.FixedWidth( 20_u ).FixedHeight( 20_u );
    b.FixedWidth( 20_u ).FixedHeight( 20_u ).SelfAlign( EAlign::Bottom );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    REQUIRE_UNIT( a.Layout.SceneRect.Origin[1],  0.f );
    REQUIRE_UNIT( b.Layout.SceneRect.Origin[1], 80.f ); // 100-20
}

TEST_CASE( "VStretch on a Horizontal child's cross axis fills the container height, overriding a Fixed height", "[layout-engine][alignment]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.LayoutType( ELayoutType::Horizontal ).ChildAlign( EAlign::VStretch )
          .FixedWidth( 200_u ).FixedHeight( 100_u );
    child.FixedWidth( 20_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    REQUIRE_UNIT( child.Layout.SceneRect.Size[1], 100.f );
}

TEST_CASE( "Cross-axis flex stretch is clamped by the child's SizeConstraints", "[layout-engine][alignment]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.LayoutType( ELayoutType::Vertical ).FixedWidth( 400_u ).FixedHeight( 100_u );
    child.FlexWidth().FixedHeight( 20_u ).SizeConstraints( Constraints::AtMost( Vec2<Unit>{ 150_u, Limits<Unit>::max() } ) );

    RunLayout( parent, Vec2<Unit>{ 400_u, 100_u }, fx );

    REQUIRE_UNIT( child.Layout.SceneRect.Size[0], 150.f );
}

// =============================================================================
// Overlay
// =============================================================================

TEST_CASE( "Overlay's default TopLeft alignment places a child at the container's origin", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ); // LayoutType defaults to Overlay
    child.FixedWidth( 30_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 0.f, 0.f, 30.f, 20.f );
}

TEST_CASE( "Overlay Center alignment centers the child within the container", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::Center );
    child.FixedWidth( 30_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 35.f, 40.f, 30.f, 20.f );
}

TEST_CASE( "Overlay BottomRight alignment anchors the child to the far corner", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::BottomRight );
    child.FixedWidth( 30_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 70.f, 80.f, 30.f, 20.f );
}

TEST_CASE( "Overlay stacks every child at its own aligned position independently", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u );
    a.FixedWidth( 40_u ).FixedHeight( 40_u );
    b.FixedWidth( 60_u ).FixedHeight( 60_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( a.Layout.SceneRect, 0.f, 0.f, 40.f, 40.f );
    REQUIRE_RECT( b.Layout.SceneRect, 0.f, 0.f, 60.f, 60.f );
}

TEST_CASE( "Overlay Flex child fills the full padded inner rect", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 80_u ).Padding( Edges::All( 5_u ) );
    child.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 100_u, 80_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 5.f, 5.f, 90.f, 70.f );
}

TEST_CASE( "Overlay resolves alignment within the margin-inset space around the child", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::TopRight );
    child.FixedWidth( 20_u ).FixedHeight( 20_u ).Margin( Edges::All( 10_u ) );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    // The 10-unit margin shrinks the alignable space to 80x80, so TopRight places the
    // child's 20x20 box against the right edge of *that* space, not the raw container.
    REQUIRE_RECT( child.Layout.SceneRect, 70.f, 10.f, 20.f, 20.f );
}

// =============================================================================
// Anchored positioning
// =============================================================================

TEST_CASE( "Anchored TopLeft with an offset positions relative to the container's top-left corner", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );

    Anchor anchor = Anchor::TopLeft();
    anchor.Offset = { 5_u, 5_u };
    child.FixedWidth( 30_u ).FixedHeight( 30_u )
         .PositionMode( EPositioning::Anchored )
         .Anchor( anchor );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 5.f, 5.f, 30.f, 30.f );
}

TEST_CASE( "Anchored Center places the child's own pivot at the container's center", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    child.FixedWidth( 40_u ).FixedHeight( 20_u )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::Center() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    // Anchor point is the container's center (100,100); the child's own center sits there.
    REQUIRE_RECT( child.Layout.SceneRect, 80.f, 90.f, 40.f, 20.f );
}

TEST_CASE( "Anchored StretchAll fills the container's padded inner rect regardless of the child's own desired size", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 150_u ).Padding( Edges::All( 10_u ) );
    child.FixedWidth( 10_u ).FixedHeight( 10_u )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::StretchAll() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 150_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 10.f, 10.f, 180.f, 130.f );
}

TEST_CASE( "An Anchored child is skipped by flow layout entirely and does not consume its Spacing", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, flowA, anchored, flowB;
    parent.PushBackChild( flowA );
    parent.PushBackChild( anchored );
    parent.PushBackChild( flowB );
    parent.LayoutType( ELayoutType::Horizontal ).Spacing( 10_u )
          .FixedWidth( 300_u ).FixedHeight( 50_u );
    flowA.FixedWidth( 50_u ).FixedHeight( 50_u );
    flowB.FixedWidth( 50_u ).FixedHeight( 50_u );
    anchored.FixedWidth( 20_u ).FixedHeight( 20_u )
            .PositionMode( EPositioning::Anchored )
            .Anchor( Anchor::TopLeft() );

    RunLayout( parent, Vec2<Unit>{ 300_u, 50_u }, fx );

    // flowB sits directly after flowA plus one Spacing gap - the anchored sibling
    // in between never touches the cursor.
    REQUIRE_UNIT( flowB.Layout.SceneRect.Origin[0], 60.f ); // 50 + 10
    REQUIRE_RECT( anchored.Layout.SceneRect, 0.f, 0.f, 20.f, 20.f );
}

TEST_CASE( "Anchored TopRight with a margin is inset from the container's top and right edges", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    child.FixedWidth( 30_u ).FixedHeight( 30_u )
         .Margin( Edges{ .T = 5_u, .R = 10_u } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::TopRight() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    // Margin shrinks the anchor container to (0,5)-(190,200); the right edge sits at 190.
    REQUIRE_RECT( child.Layout.SceneRect, 160.f, 5.f, 30.f, 30.f );
}

TEST_CASE( "Anchored StretchAll with an uneven margin insets each edge independently, inside the parent's padding", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 150_u ).Padding( Edges::All( 10_u ) );
    child.Margin( Edges{ .T = 0_u, .R = 15_u, .B = 20_u, .L = 5_u } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::StretchAll() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 150_u }, fx );

    // Padded inner rect (10,10,180,130), then margin L5 R15 T0 B20.
    REQUIRE_RECT( child.Layout.SceneRect, 15.f, 10.f, 160.f, 110.f );
}

TEST_CASE( "Anchored Percent sizing is relative to the parent's padded inner rect", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 100_u ).Padding( Edges::All( 10_u ) );
    child.PercentWidth( 1.0f ).FixedHeight( 20_u )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::TopLeft() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    // 100% of the 180-wide inner rect, not the 200-wide outer size.
    REQUIRE_UNIT( child.Layout.DesiredSize[0], 180.f );
    REQUIRE_RECT( child.Layout.SceneRect, 10.f, 10.f, 180.f, 20.f );
}

TEST_CASE( "Anchored StretchAll respects SizeConstraints and places the clamped node by its pivot", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 100_u );
    child.SizeConstraints( Constraints{ .Min = { 0_u, 0_u }, .Max = { 100_u, Limits<Unit>::max() } } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::StretchAll() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    // Width clamps 200 -> 100 and the 0.5 pivot centers it within the stretched span.
    REQUIRE_RECT( child.Layout.SceneRect, 50.f, 0.f, 100.f, 100.f );
}

TEST_CASE( "Anchored StretchAll applies Offset on top of an uneven margin", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );

    Anchor anchor = Anchor::StretchAll();
    anchor.Offset = { 5_u, 5_u };
    child.Margin( Edges{ .L = 10_u } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( anchor );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    // Margin gives a (10,0)-(200,200) container; the offset then insets each stretched edge by 5.
    REQUIRE_RECT( child.Layout.SceneRect, 15.f, 5.f, 180.f, 190.f );
}

TEST_CASE( "Anchored Percent sizing is still clamped by SizeConstraints", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    child.PercentWidth( 1.0f ).FixedHeight( 20_u )
         .SizeConstraints( Constraints{ .Min = { 0_u, 0_u }, .Max = { 100_u, Limits<Unit>::max() } } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::TopLeft() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_RECT( child.Layout.SceneRect, 0.f, 0.f, 100.f, 20.f );
}

TEST_CASE( "Anchored StretchAll with a Min larger than the span starts at the span's start", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 200_u ).FixedHeight( 100_u );
    child.SizeConstraints( Constraints{ .Min = { 300_u, 0_u }, .Max = { Limits<Unit>::max(), Limits<Unit>::max() } } )
         .PositionMode( EPositioning::Anchored )
         .Anchor( Anchor::StretchAll() );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    // Overflows past the right edge rather than being shifted left of the container by the pivot.
    REQUIRE_RECT( child.Layout.SceneRect, 0.f, 0.f, 300.f, 100.f );
}

TEST_CASE( "Anchored Flex sizing is treated as Content sizing", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode parent, anchored, content;
    parent.PushBackChild( anchored );
    anchored.PushBackChild( content );
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    anchored.WidthMode( ESizing::Flex ).FixedHeight( 20_u )
            .PositionMode( EPositioning::Anchored )
            .Anchor( Anchor::TopLeft() );
    content.FixedWidth( 40_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );

    // No flow space to share, so Flex sizes to its children rather than filling or collapsing.
    REQUIRE_RECT( anchored.Layout.SceneRect, 0.f, 0.f, 40.f, 20.f );
}

// =============================================================================
// Grid
// =============================================================================

TEST_CASE( "Grid with only GridColumns set auto-calculates the row count and sizes tracks from content", "[layout-engine][grid]" )
{
    LayoutFixture fx;
    LayoutNode parent, c0, c1, c2, c3, c4;
    for ( LayoutNode* c : { &c0, &c1, &c2, &c3, &c4 } )
    {
        parent.PushBackChild( *c );
        c->FixedWidth( 20_u ).FixedHeight( 10_u );
    }
    parent.LayoutType( ELayoutType::Grid ).GridColumns( 3 ).GridRows( 0 );

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 400_u, 400_u }, fx.Ctx );

    // 5 children over 3 columns -> 2 rows. Every child is the same size, so each of the
    // 3 columns is 20 wide and each of the 2 rows is 10 tall: 3*20 x 2*10.
    REQUIRE_VEC2( desired, 60.f, 20.f );
}

TEST_CASE( "Grid with both GridColumns and GridRows explicitly set divides the container evenly, ignoring intrinsic child size", "[layout-engine][grid]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b, c, d;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );
    parent.PushBackChild( d );
    parent.LayoutType( ELayoutType::Grid ).GridColumns( 2 ).GridRows( 2 )
          .ChildAlign( EAlign::StretchFill )
          .FixedWidth( 200_u ).FixedHeight( 100_u );
    a.FixedWidth( 10_u ).FixedHeight( 10_u );
    b.FixedWidth( 10_u ).FixedHeight( 10_u );
    c.FixedWidth( 10_u ).FixedHeight( 10_u );
    d.FixedWidth( 10_u ).FixedHeight( 10_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 100_u }, fx );

    REQUIRE_RECT( a.Layout.SceneRect,   0.f,  0.f, 100.f, 50.f );
    REQUIRE_RECT( b.Layout.SceneRect, 100.f,  0.f, 100.f, 50.f );
    REQUIRE_RECT( c.Layout.SceneRect,   0.f, 50.f, 100.f, 50.f );
    REQUIRE_RECT( d.Layout.SceneRect, 100.f, 50.f, 100.f, 50.f );
}

TEST_CASE( "Grid without both dimensions explicitly set sizes each track from its largest child, not evenly", "[layout-engine][grid]" )
{
    LayoutFixture fx;
    LayoutNode parent, a, b, c, d;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );
    parent.PushBackChild( d );
    parent.LayoutType( ELayoutType::Grid ).GridColumns( 2 ).GridRows( 0 )
          .FixedWidth( 400_u ).FixedHeight( 400_u );
    a.FixedWidth( 30_u ).FixedHeight( 10_u );
    b.FixedWidth( 50_u ).FixedHeight( 20_u );
    c.FixedWidth( 10_u ).FixedHeight( 40_u );
    d.FixedWidth( 10_u ).FixedHeight( 10_u );

    RunLayout( parent, Vec2<Unit>{ 400_u, 400_u }, fx );

    // Column 0 width = max(a,c) = 30; column 1 width = max(b,d) = 50.
    // Row 0 height = max(a,b) = 20; row 1 height = max(c,d) = 40.
    // Default TopLeft alignment leaves each child at its own intrinsic size within its cell.
    REQUIRE_RECT( a.Layout.SceneRect,  0.f,  0.f, 30.f, 10.f );
    REQUIRE_RECT( b.Layout.SceneRect, 30.f,  0.f, 50.f, 20.f );
    REQUIRE_RECT( c.Layout.SceneRect,  0.f, 20.f, 10.f, 40.f );
    REQUIRE_RECT( d.Layout.SceneRect, 30.f, 20.f, 10.f, 10.f );
}

// =============================================================================
// Visibility interacting with layout
// =============================================================================

TEST_CASE( "A root node with no parent resolves its visibility directly from its own style", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode root;
    root.Visibility( EVisibility::Hidden );
    root.Layout.Visibility = EVisibility::Visible; // As if not resolved yet.

    MeasureLayoutNode( root, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );

    REQUIRE( root.Layout.Visibility == EVisibility::Hidden );
}

TEST_CASE( "A Collapsed child is skipped entirely during Measure and never contributes to its parent's content size", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode parent, visible, collapsed;
    parent.PushBackChild( visible );
    parent.PushBackChild( collapsed );
    parent.LayoutType( ELayoutType::Horizontal );
    visible.FixedWidth( 50_u ).FixedHeight( 50_u );
    collapsed.FixedWidth( 999_u ).FixedHeight( 999_u ).Visibility( EVisibility::Collapsed );

    // Sentinel value proving the collapsed child is skipped outright, not measured-and-zeroed.
    collapsed.Layout.DesiredSize = Vec2<Unit>{ 12345_u, 12345_u };

    Vec2<Unit> desired = MeasureLayoutNode( parent, Vec2<Unit>{ 1000_u, 1000_u }, fx.Ctx );

    REQUIRE_VEC2( desired, 50.f, 50.f );
    REQUIRE_VEC2( collapsed.Layout.DesiredSize, 12345.f, 12345.f );
}

TEST_CASE( "A Hidden child still occupies layout space and gets arranged; it just isn't rendered or hit-testable", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode parent, visible, hidden;
    parent.PushBackChild( visible );
    parent.PushBackChild( hidden );
    parent.LayoutType( ELayoutType::Horizontal ).FixedWidth( 200_u ).FixedHeight( 50_u );
    visible.FixedWidth( 50_u ).FixedHeight( 50_u );
    hidden.FixedWidth( 30_u ).FixedHeight( 30_u ).Visibility( EVisibility::Hidden );

    RunLayout( parent, Vec2<Unit>{ 200_u, 50_u }, fx );

    REQUIRE_RECT( hidden.Layout.SceneRect, 50.f, 0.f, 30.f, 30.f );
    REQUIRE_FALSE( Visibility::IsRendered( hidden.Layout.Visibility ) );
    REQUIRE( Visibility::AffectsLayout( hidden.Layout.Visibility ) );
}

TEST_CASE( "A Collapsed node's subtree takes no space, and everything in it reads as collapsed", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode root, collapsedParent, child, sibling;
    root.LayoutType( ELayoutType::Vertical );
    root.PushBackChild( collapsedParent );
    root.PushBackChild( sibling );
    collapsedParent.PushBackChild( child );
    collapsedParent.Visibility( EVisibility::Collapsed );
    child.FixedWidth( 40_u ).FixedHeight( 40_u );
    sibling.FixedWidth( 10_u ).FixedHeight( 10_u );

    RunLayout( root, Vec2<Unit>{ 500_u, 500_u }, fx );

    REQUIRE_FALSE( Visibility::AffectsLayout( collapsedParent.Layout.Visibility ) );
    REQUIRE( child.Layout.Visibility == EVisibility::Collapsed );
    REQUIRE_FALSE( child.Layout.HitTestable );
    REQUIRE_UNIT( sibling.Layout.ParentRect.Origin[1], 0.f ); // Nothing above it takes space.
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );

    // Shown again, the subtree is laid out and visible.
    collapsedParent.Visibility( EVisibility::Visible );
    RunLayout( root, Vec2<Unit>{ 500_u, 500_u }, fx );
    REQUIRE( child.Layout.Visibility == EVisibility::Visible );
    REQUIRE_RECT( child.Layout.SceneRect, 0.f, 0.f, 40.f, 40.f );
    REQUIRE_UNIT( sibling.Layout.ParentRect.Origin[1], 40.f );
}

TEST_CASE( "A Collapsed root settles: it isn't measured again until it changes", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode    root;
    root.Visibility( EVisibility::Collapsed );

    RunLayout( root, Vec2<Unit>{ 100_u, 100_u }, fx );
    REQUIRE_FALSE( root.Layout.IsDirty );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );
    REQUIRE_VEC2( root.Layout.DesiredSize, 0.f, 0.f );
}

TEST_CASE( "Visibility setter: only entering or leaving Collapsed runs layout again", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode    root, child;
    root.PushBackChild( child );
    RunLayout( root, Vec2<Unit>{ 100_u, 100_u }, fx );

    child.Visibility( EVisibility::Hidden );
    REQUIRE_FALSE( child.Layout.IsDirty );
    REQUIRE( child.Layout.Visibility == EVisibility::Hidden ); // Resolved at once, without a layout.

    child.Visibility( EVisibility::Collapsed );
    REQUIRE( child.Layout.IsDirty );
    RunLayout( root, Vec2<Unit>{ 100_u, 100_u }, fx );

    child.Visibility( EVisibility::Hidden );
    REQUIRE( child.Layout.IsDirty );
    RunLayout( root, Vec2<Unit>{ 100_u, 100_u }, fx );

    child.Visibility( EVisibility::Visible );
    REQUIRE_FALSE( child.Layout.IsDirty );
}

TEST_CASE( "A child can't be more visible or hit-testable than its parent", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode    root, child;
    root.PushBackChild( child );
    RunLayout( root, Vec2<Unit>{ 100_u, 100_u }, fx );

    root.Visibility( EVisibility::Hidden );
    child.Visibility( EVisibility::Visible );
    REQUIRE( child.Layout.Visibility == EVisibility::Hidden );
    REQUIRE_FALSE( child.Layout.HitTestable );

    root.Visibility( EVisibility::Visible ).HitTestable( false );
    child.HitTestable( true );
    REQUIRE( child.Layout.Visibility == EVisibility::Visible );
    REQUIRE_FALSE( child.Layout.HitTestable );

    root.HitTestable( true );
    REQUIRE( child.Layout.HitTestable );
}

// =============================================================================
// Measure's dirty-based memoization
// =============================================================================

TEST_CASE( "MeasureLayoutNode skips recomputation when nothing is dirty and the available size is unchanged", "[layout-engine][memoization]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.FixedWidth( 50_u ).FixedHeight( 50_u );

    MeasureLayoutNode( node, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );
    REQUIRE_FALSE( node.Layout.IsDirty );

    // Poison the cached value directly: if Measure genuinely skips recomputation,
    // it hands this poisoned value straight back out instead of recomputing 50x50.
    node.Layout.DesiredSize = Vec2<Unit>{ 777_u, 777_u };

    Vec2<Unit> result = MeasureLayoutNode( node, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );

    REQUIRE_VEC2( result, 777.f, 777.f );
}

TEST_CASE( "MeasureLayoutNode recomputes when the node has been explicitly marked dirty", "[layout-engine][memoization]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.FixedWidth( 50_u ).FixedHeight( 50_u );
    MeasureLayoutNode( node, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );

    node.Layout.DesiredSize = Vec2<Unit>{ 777_u, 777_u };
    node.MarkDirty();

    Vec2<Unit> result = MeasureLayoutNode( node, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );

    REQUIRE_VEC2( result, 50.f, 50.f ); // recomputed from the Fixed style, not the poisoned cache
}

TEST_CASE( "MeasureLayoutNode recomputes when the available size changes even if nothing is marked dirty", "[layout-engine][memoization]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.PercentWidth( 0.5f ).PercentHeight( 0.5f );

    MeasureLayoutNode( node, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );
    REQUIRE_VEC2( node.Layout.DesiredSize, 50.f, 50.f );

    Vec2<Unit> result = MeasureLayoutNode( node, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );

    REQUIRE_VEC2( result, 100.f, 100.f );
}

TEST_CASE( "Marking a leaf dirty forces the whole ancestor chain to recompute on the next Measure", "[layout-engine][memoization]" )
{
    LayoutFixture fx;
    LayoutNode root, mid, leaf;
    root.PushBackChild( mid );
    mid.PushBackChild( leaf );
    root.LayoutType( ELayoutType::Vertical );
    mid.LayoutType( ELayoutType::Vertical );
    leaf.FixedWidth( 20_u ).FixedHeight( 20_u );

    MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );

    leaf.FixedHeight( 40_u ); // factory setters call MarkDirty() internally

    REQUIRE( root.Layout.IsDescendantDirty );

    Vec2<Unit> result = MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );

    REQUIRE_UNIT( result[1], 40.f );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty ); // recompute clears the flag again
}

TEST_CASE( "The safe area is resolved during measure, so one arrange applies it with no reflow", "[layout-engine][safe-area]" )
{
    LayoutFixture fx;
    LayoutNode root, child;
    root.PushBackChild( child );
    root.SafeAreaEdges( ESafeAreaEdges::All );
    child.FixedWidth( 50_u ).FixedHeight( 30_u );

    fx.Ctx.RemainingSafeArea = Edges{ .T = 40_u, .L = 10_u };
    MeasureLayoutNode( root, Vec2<Unit>{ 400_u, 300_u }, fx.Ctx );
    const bool reflowed = ArrangeLayoutNode( root, Rect<Unit>{ Vec2<Unit>{ 0_u, 0_u }, Vec2<Unit>{ 400_u, 300_u } }, fx.Ctx );

    REQUIRE_FALSE( reflowed );
    REQUIRE_RECT( child.Layout.ParentRect, 10.f, 40.f, 50.f, 30.f );
}

TEST_CASE( "MeasureLayoutNode recomputes when the safe area changes even if nothing is marked dirty", "[layout-engine][memoization][safe-area]" )
{
    LayoutFixture fx;
    LayoutNode root, child;
    root.PushBackChild( child );
    root.SafeAreaEdges( ESafeAreaEdges::Top );
    child.FixedWidth( 50_u ).FixedHeight( 30_u );

    REQUIRE_VEC2( MeasureLayoutNode( root, Vec2<Unit>{ 400_u, 300_u }, fx.Ctx ), 50.f, 30.f );

    fx.Ctx.RemainingSafeArea = Edges{ .T = 20_u };
    REQUIRE_VEC2( MeasureLayoutNode( root, Vec2<Unit>{ 400_u, 300_u }, fx.Ctx ), 50.f, 50.f );

    // The context is left as the caller set it, not as the last child saw it.
    REQUIRE( fx.Ctx.RemainingSafeArea == Edges{ .T = 20_u } );
}

TEST_CASE( "ContentPadding includes the safe area a node clears, and Apply never turns a rect inside out", "[layout-engine][safe-area]" )
{
    LayoutFixture fx;
    LayoutNode node;
    node.Padding( Edges::All( 5_u ) ).SafeAreaEdges( ESafeAreaEdges::Top ).FixedWidth( 100_u ).FixedHeight( 30_u );

    fx.Ctx.RemainingSafeArea = Edges{ .T = 40_u, .L = 10_u };
    MeasureLayoutNode( node, Vec2<Unit>{ 400_u, 300_u }, fx.Ctx );

    // Text widgets paint inside ContentPadding, so this is the rect their text lands in.
    const Edges padding = ContentPadding( node );
    REQUIRE( padding == Edges{ .T = 45_u, .R = 5_u, .B = 5_u, .L = 5_u } );

    // 45 + 5 of vertical padding on a 30-tall node: zero height at the inset top, not negative.
    const Rect<Unit> inner = padding.Apply( Rect<Unit>{ Vec2<Unit>{ 0_u, 0_u }, Vec2<Unit>{ 100_u, 30_u } } );
    REQUIRE_RECT( inner, 5.f, 45.f, 90.f, 0.f );
}

// =============================================================================
// Anchored: margin with percent sizes, flex height, ignored alignment
// =============================================================================

TEST_CASE( "Anchored Percent sizes are relative to the container less the margin", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode    parent, child;
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    parent.PushBackChild( child );
    child.PositionMode( EPositioning::Anchored ).Anchor( Anchor::TopLeft() )
         .PercentWidth( 0.5f ).FixedHeight( 10_u ).Margin( Edges{ .R = 20_u, .L = 20_u } );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_RECT( child.Layout.ParentRect, 20.f, 0.f, 80.f, 10.f );
}

TEST_CASE( "Anchored Flex height sizes to the content, like Flex width", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode    parent, child, content;
    parent.FixedWidth( 200_u ).FixedHeight( 200_u );
    parent.PushBackChild( child );
    child.PositionMode( EPositioning::Anchored ).Anchor( Anchor::TopLeft() ).FixedWidth( 50_u ).FlexHeight();
    child.PushBackChild( content );
    content.FixedWidth( 10_u ).FixedHeight( 30_u );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_RECT( child.Layout.ParentRect, 0.f, 0.f, 50.f, 30.f );
}

TEST_CASE( "Anchored nodes ignore SelfAlign and the parent's ChildAlign", "[layout-engine][anchor]" )
{
    LayoutFixture fx;
    LayoutNode    parent, child;
    parent.FixedWidth( 200_u ).FixedHeight( 200_u ).ChildAlign( EAlign::Center );
    parent.PushBackChild( child );
    child.PositionMode( EPositioning::Anchored ).Anchor( Anchor::TopLeft() )
         .FixedWidth( 20_u ).FixedHeight( 20_u ).SelfAlign( EAlign::BottomRight );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_RECT( child.Layout.ParentRect, 0.f, 0.f, 20.f, 20.f );
}

TEST_CASE( "A Collapsed anchored child takes no space in a linear layout", "[layout-engine][anchor][visibility]" )
{
    LayoutFixture fx;
    LayoutNode    parent, child;
    parent.LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u ).FixedHeight( 200_u );
    parent.PushBackChild( child );
    child.PositionMode( EPositioning::Anchored ).FixedWidth( 30_u ).FixedHeight( 30_u ).Visibility( EVisibility::Collapsed );

    RunLayout( parent, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_VEC2( child.Layout.ParentRect.Size, 0.f, 0.f );
    REQUIRE_VEC2( parent.Layout.ContentSize, 0.f, 0.f );
}

// =============================================================================
// Reflow: content whose height depends on its final width
// =============================================================================

namespace
{
    /** @brief A widget whose height is 1000 / width, the way wrapped text gets taller as it narrows. */
    struct WrappingProbe : IWidget
    {
        WrappingProbe() : IWidget( { .WidthDependentContent = true } ) {}

        Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit> a_Available, const LayoutContext& ) override
        {
            const f32 width = std::max( 1.f, std::min( a_Available[0].ToFloat(), 100.f ) );
            return { Unit{ width }, Unit{ 1000.f / width } };
        }
    };
}

TEST_CASE( "Arrange asks for another pass when a node's final width changes its content height", "[layout-engine][reflow]" )
{
    // Measured at the full 200 width (capped at 100: 10 tall), arranged at its 40 max width (25 tall).
    LayoutFixture fx;
    LayoutNode    root, panel, wrapping;
    root.LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u );
    panel.LayoutType( ELayoutType::Vertical ).FlexWidth();
    root.PushBackChild( panel );
    panel.PushBackChild( wrapping );
    wrapping.Widget = MakeUnique<WrappingProbe>();
    wrapping.FlexWidth().SizeConstraints( Constraints{ .Max = { 40_u, Limits<Unit>::max() } } );

    MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
    REQUIRE( ArrangeLayoutNode( root, Rect<Unit>{ { 0_u, 0_u }, { 200_u, 200_u } }, fx.Ctx ) );
    REQUIRE_UNIT( wrapping.Layout.DesiredSize[1], 25.f );
    REQUIRE( root.Layout.IsDescendantDirty ); // The next pass measures the ancestors again.

    // The second pass grows the content-height panel to fit, and settles.
    MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
    REQUIRE_FALSE( ArrangeLayoutNode( root, Rect<Unit>{ { 0_u, 0_u }, { 200_u, 200_u } }, fx.Ctx ) );
    REQUIRE_UNIT( panel.Layout.ParentRect.Size[1], 25.f );
    REQUIRE_UNIT( wrapping.Layout.ParentRect.Size[1], 25.f );
}

TEST_CASE( "Reflow honours a fixed height and the size constraints", "[layout-engine][reflow]" )
{
    LayoutFixture fx;
    LayoutNode    root, wrapping;
    root.LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u );
    root.PushBackChild( wrapping );
    wrapping.Widget = MakeUnique<WrappingProbe>();
    wrapping.FlexWidth();

    SECTION( "Fixed height: nothing to reflow" )
    {
        wrapping.FixedHeight( 12_u ).SizeConstraints( Constraints{ .Max = { 40_u, Limits<Unit>::max() } } );
        MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
        REQUIRE_FALSE( ArrangeLayoutNode( root, Rect<Unit>{ { 0_u, 0_u }, { 200_u, 200_u } }, fx.Ctx ) );
        REQUIRE_UNIT( wrapping.Layout.ParentRect.Size[1], 12.f );
    }

    SECTION( "The new height is clamped to the constraints" )
    {
        wrapping.SizeConstraints( Constraints{ .Max = { 40_u, 18_u } } );
        MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
        REQUIRE( ArrangeLayoutNode( root, Rect<Unit>{ { 0_u, 0_u }, { 200_u, 200_u } }, fx.Ctx ) );
        REQUIRE_UNIT( wrapping.Layout.DesiredSize[1], 18.f );
    }
}

TEST_CASE( "A Collapsed anchored subtree is not arranged, so its content doesn't reflow", "[layout-engine][anchor][visibility][reflow]" )
{
    LayoutFixture fx;
    LayoutNode    root, panel, wrapping;
    root.LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u ).FixedHeight( 200_u );
    root.PushBackChild( panel );
    panel.PushBackChild( wrapping );
    panel.PositionMode( EPositioning::Anchored ).FixedWidth( 40_u ).Visibility( EVisibility::Collapsed );
    wrapping.Widget = MakeUnique<WrappingProbe>();
    wrapping.FlexWidth();

    MeasureLayoutNode( root, Vec2<Unit>{ 200_u, 200_u }, fx.Ctx );
    REQUIRE_FALSE( ArrangeLayoutNode( root, Rect<Unit>{ { 0_u, 0_u }, { 200_u, 200_u } }, fx.Ctx ) );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );
    REQUIRE_VEC2( wrapping.Layout.ParentRect.Size, 0.f, 0.f );
}

TEST_CASE( "SizeConstraints hold for flex and percent children of an overlay and a grid", "[layout-engine][sizing]" )
{
    LayoutFixture fx;
    LayoutNode parent, flex, percent;
    parent.PushBackChild( flex );
    parent.PushBackChild( percent );
    parent.FixedWidth( 400_u ).FixedHeight( 300_u );
    flex.FlexWidth().FlexHeight().SizeConstraints( Constraints::AtMost( Vec2<Unit>{ 100_u, 50_u } ) );
    percent.PercentWidth( 0.5f ).FixedHeight( 20_u ).SizeConstraints( Constraints::AtMost( Vec2<Unit>{ 80_u, Limits<Unit>::max() } ) );

    SECTION( "overlay" )
    {
        parent.LayoutType( ELayoutType::Overlay );
        RunLayout( parent, Vec2<Unit>{ 400_u, 300_u }, fx );
        REQUIRE_VEC2( flex.Layout.ParentRect.Size, 100.f, 50.f );
    }
    SECTION( "grid" )
    {
        // A flex child here only fills a cell its neighbours size, so the percent child is the one to check.
        parent.LayoutType( ELayoutType::Grid );
        RunLayout( parent, Vec2<Unit>{ 400_u, 300_u }, fx );
    }

    REQUIRE( percent.Layout.ParentRect.Size[0].ToFloat() == Catch::Approx( 80.f ) );
}
