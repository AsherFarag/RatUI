/**
 * @file TestLayoutEngine.cpp
 * @brief Tests for MeasureLayoutNode / ArrangeLayoutNode: sizing modes, padding/margin/spacing,
 *        flex distribution, alignment, overlay stacking, anchored positioning, grid layout,
 *        how EVisibility interacts with layout, and Measure's dirty-based memoization.
 *
 * All trees here are built from plain, widget-less LayoutNodes (a_Node.Widget == nullptr), so
 * content sizing comes purely from children/padding, never from IWidget::OnMeasureContent, and
 * ArrangeLayoutNode's reflow path (HasWidthDependentContent) never triggers.
 */

#include "TestLayoutCommon.h"

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

    REQUIRE_RECT( child.Layout.FinalRect, 20.f, 20.f, 160.f, 60.f );
}

TEST_CASE( "Margin insets a child within the space its parent allocated to it", "[layout-engine][margin]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u );
    child.FlexWidth().FlexHeight().Margin( Edges::All( 10_u ) );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.FinalRect, 10.f, 10.f, 80.f, 80.f );
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

    REQUIRE_RECT( a.Layout.FinalRect,  0.f, 0.f, 50.f, 50.f );
    REQUIRE_RECT( b.Layout.FinalRect, 60.f, 0.f, 60.f, 60.f ); // 50 + 10 spacing
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

    REQUIRE_RECT( a.Layout.FinalRect, 0.f,  0.f, 30.f, 40.f );
    REQUIRE_RECT( b.Layout.FinalRect, 0.f, 45.f, 30.f, 50.f ); // 40 + 5 spacing
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
    REQUIRE_RECT( a.Layout.FinalRect,   0.f, 0.f, 100.f, 100.f );
    REQUIRE_RECT( b.Layout.FinalRect, 100.f, 0.f, 100.f, 100.f );
    REQUIRE_RECT( c.Layout.FinalRect, 200.f, 0.f, 200.f, 100.f );
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

    REQUIRE_UNIT( a.Layout.FinalRect.Size[0], 100.f );
    REQUIRE_UNIT( b.Layout.FinalRect.Size[0], 100.f );
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

    REQUIRE_UNIT( flex.Layout.FinalRect.Size[0], 200.f ); // 300 - 100
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
    REQUIRE_RECT( a.Layout.FinalRect,  0.f, 0.f,  80.f, 50.f );
    REQUIRE_RECT( b.Layout.FinalRect, 80.f, 0.f, 200.f, 50.f );
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

    REQUIRE_UNIT( child.Layout.FinalRect.Origin[1], 40.f ); // (100-20)/2
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

    REQUIRE_UNIT( a.Layout.FinalRect.Origin[1],  0.f );
    REQUIRE_UNIT( b.Layout.FinalRect.Origin[1], 80.f ); // 100-20
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

    REQUIRE_UNIT( child.Layout.FinalRect.Size[1], 100.f );
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

    REQUIRE_RECT( child.Layout.FinalRect, 0.f, 0.f, 30.f, 20.f );
}

TEST_CASE( "Overlay Center alignment centers the child within the container", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::Center );
    child.FixedWidth( 30_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.FinalRect, 35.f, 40.f, 30.f, 20.f );
}

TEST_CASE( "Overlay BottomRight alignment anchors the child to the far corner", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 100_u ).ChildAlign( EAlign::BottomRight );
    child.FixedWidth( 30_u ).FixedHeight( 20_u );

    RunLayout( parent, Vec2<Unit>{ 100_u, 100_u }, fx );

    REQUIRE_RECT( child.Layout.FinalRect, 70.f, 80.f, 30.f, 20.f );
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

    REQUIRE_RECT( a.Layout.FinalRect, 0.f, 0.f, 40.f, 40.f );
    REQUIRE_RECT( b.Layout.FinalRect, 0.f, 0.f, 60.f, 60.f );
}

TEST_CASE( "Overlay Flex child fills the full padded inner rect", "[layout-engine][overlay]" )
{
    LayoutFixture fx;
    LayoutNode parent, child;
    parent.PushBackChild( child );
    parent.FixedWidth( 100_u ).FixedHeight( 80_u ).Padding( Edges::All( 5_u ) );
    child.FlexWidth().FlexHeight();

    RunLayout( parent, Vec2<Unit>{ 100_u, 80_u }, fx );

    REQUIRE_RECT( child.Layout.FinalRect, 5.f, 5.f, 90.f, 70.f );
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
    REQUIRE_RECT( child.Layout.FinalRect, 70.f, 10.f, 20.f, 20.f );
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

    REQUIRE_RECT( child.Layout.FinalRect, 5.f, 5.f, 30.f, 30.f );
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
    REQUIRE_RECT( child.Layout.FinalRect, 80.f, 90.f, 40.f, 20.f );
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

    REQUIRE_RECT( child.Layout.FinalRect, 10.f, 10.f, 180.f, 130.f );
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
    REQUIRE_UNIT( flowB.Layout.FinalRect.Origin[0], 60.f ); // 50 + 10
    REQUIRE_RECT( anchored.Layout.FinalRect, 0.f, 0.f, 20.f, 20.f );
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

    REQUIRE_RECT( a.Layout.FinalRect,   0.f,  0.f, 100.f, 50.f );
    REQUIRE_RECT( b.Layout.FinalRect, 100.f,  0.f, 100.f, 50.f );
    REQUIRE_RECT( c.Layout.FinalRect,   0.f, 50.f, 100.f, 50.f );
    REQUIRE_RECT( d.Layout.FinalRect, 100.f, 50.f, 100.f, 50.f );
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
    REQUIRE_RECT( a.Layout.FinalRect,  0.f,  0.f, 30.f, 10.f );
    REQUIRE_RECT( b.Layout.FinalRect, 30.f,  0.f, 50.f, 20.f );
    REQUIRE_RECT( c.Layout.FinalRect,  0.f, 20.f, 10.f, 40.f );
    REQUIRE_RECT( d.Layout.FinalRect, 30.f, 20.f, 10.f, 10.f );
}

// =============================================================================
// Visibility interacting with layout
// =============================================================================

TEST_CASE( "A root node with no parent resolves its visibility directly from its own style", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode root;
    root.Visibility( EVisibility::HitTestInvisible );

    MeasureLayoutNode( root, Vec2<Unit>{ 100_u, 100_u }, fx.Ctx );

    REQUIRE( root.Layout.Visibility == EVisibility::HitTestInvisible );
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

    REQUIRE_RECT( hidden.Layout.FinalRect, 50.f, 0.f, 30.f, 30.f );
    REQUIRE_FALSE( Visibility::IsRendered( hidden.Layout.Visibility ) );
    REQUIRE( Visibility::AffectsLayout( hidden.Layout.Visibility ) );
}

TEST_CASE( "A Collapsed node's subtree is pruned entirely - descendants are never visited or re-resolved", "[layout-engine][visibility]" )
{
    LayoutFixture fx;
    LayoutNode root, collapsedParent, child;
    root.PushBackChild( collapsedParent );
    collapsedParent.PushBackChild( child );
    collapsedParent.Visibility( EVisibility::Collapsed );
    child.FixedWidth( 40_u ).FixedHeight( 40_u );

    RunLayout( root, Vec2<Unit>{ 500_u, 500_u }, fx );

    // collapsedParent itself is resolved to Collapsed and excluded from layout...
    REQUIRE_FALSE( Visibility::AffectsLayout( collapsedParent.Layout.Visibility ) );
    // ...but because its subtree is pruned, `child` is never visited at all: its cached
    // Layout.Visibility is left exactly as it was before this pass (the default Visible),
    // not resolved to Collapsed, and it never receives a FinalRect from this Arrange pass.
    REQUIRE( child.Layout.Visibility == EVisibility::Visible );
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
