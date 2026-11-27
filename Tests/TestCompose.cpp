#include "TestLayoutCommon.h"

/**
 * @file TestCompose.cpp
 * @brief ParentRect, SceneRect, ContentSize and ContentOffset: what arrange leaves in each node's own space, and how
 *        ComposeLayout turns it into where things are drawn.
 */

namespace
{
    /** @brief A padded 200x200 vertical root, a padded 100x50 panel in it, and a 20x20 leaf in the panel. */
    struct NestedFixture
    {
        LayoutFixture Fx;
        LayoutNode    Root, Panel, Leaf;

        NestedFixture()
        {
            Root.LayoutType( ELayoutType::Vertical ).Padding( Edges::All( 10_u ) );
            Panel.FixedWidth( 100_u ).FixedHeight( 50_u ).Padding( Edges::All( 5_u ) );
            Leaf.FixedWidth( 20_u ).FixedHeight( 20_u );
            Root.PushBackChild( Panel );
            Panel.PushBackChild( Leaf );
            RunLayout( Root, Vec2<Unit>{ 200_u, 200_u }, Fx );
        }
    };
}

TEST_CASE( "ParentRect is in the parent's space and SceneRect in the scene's", "[compose]" )
{
    NestedFixture f;

    REQUIRE_RECT( f.Panel.Layout.ParentRect, 10.f, 10.f, 100.f, 50.f );
    REQUIRE_RECT( f.Panel.Layout.SceneRect,  10.f, 10.f, 100.f, 50.f );

    REQUIRE_RECT( f.Leaf.Layout.ParentRect,  5.f,  5.f, 20.f, 20.f );
    REQUIRE_RECT( f.Leaf.Layout.SceneRect,  15.f, 15.f, 20.f, 20.f );
}

TEST_CASE( "A content offset moves what is below a node without touching layout", "[compose]" )
{
    NestedFixture f;

    SetContentOffset( f.Root, Vec2<Unit>{ 3_u, -4_u } );

    REQUIRE_RECT( f.Panel.Layout.SceneRect, 13.f,  6.f, 100.f, 50.f );
    REQUIRE_RECT( f.Leaf.Layout.SceneRect,  18.f, 11.f,  20.f, 20.f );
    REQUIRE_RECT( f.Root.Layout.SceneRect,   0.f,  0.f, 200.f, 200.f ); // Its children move, not the node.

    REQUIRE_RECT( f.Panel.Layout.ParentRect, 10.f, 10.f, 100.f, 50.f );
    REQUIRE_RECT( f.Leaf.Layout.ParentRect,   5.f,  5.f,  20.f, 20.f );
    REQUIRE_FALSE( f.Root.Layout.IsDirty );
    REQUIRE_FALSE( f.Root.Layout.IsDescendantDirty );

    // Offsets on nested nodes add up, and survive the next layout.
    SetContentOffset( f.Panel, Vec2<Unit>{ 0_u, -2_u } );
    REQUIRE_RECT( f.Leaf.Layout.SceneRect, 18.f, 9.f, 20.f, 20.f );

    f.Leaf.MarkDirty();
    RunLayout( f.Root, Vec2<Unit>{ 200_u, 200_u }, f.Fx );
    REQUIRE_RECT( f.Leaf.Layout.SceneRect, 18.f, 9.f, 20.f, 20.f );
}

TEST_CASE( "ContentSize is how far a node's children extend, with its end padding", "[compose]" )
{
    LayoutFixture fx;
    LayoutNode    root, a, b;
    root.LayoutType( ELayoutType::Vertical ).Padding( Edges::All( 10_u ) ).FixedWidth( 80_u ).FixedHeight( 60_u );
    a.FixedWidth( 100_u ).FixedHeight( 50_u );
    b.FixedWidth( 40_u ).FixedHeight( 50_u );
    root.PushBackChild( a );
    root.PushBackChild( b );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    // Wider and taller than the node itself: the children overflow it.
    REQUIRE_VEC2( root.Layout.ContentSize, 10.f + 100.f + 10.f, 10.f + 50.f + 50.f + 10.f );
    REQUIRE_VEC2( a.Layout.ContentSize, 0.f, 0.f );

    // A collapsed child doesn't count.
    b.Visibility( EVisibility::Collapsed );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_VEC2( root.Layout.ContentSize, 120.f, 10.f + 50.f + 10.f );
}

TEST_CASE( "Arrange reports a size or content size change only for nodes that ask", "[compose]" )
{
    LayoutFixture         fx;
    Array<GeometryChange> changes;
    fx.Ctx.GeometryChanges = &changes;

    LayoutNode root, watched, plain;
    root.LayoutType( ELayoutType::Vertical );
    watched.FixedWidth( 30_u ).FixedHeight( 30_u );
    plain.FixedWidth( 30_u ).FixedHeight( 30_u );
    watched.NotifyGeometry = true;
    root.PushBackChild( watched );
    root.PushBackChild( plain );

    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE( Size( changes ) == 1 );
    REQUIRE( changes[0].Node == &watched );
    REQUIRE_VEC2( changes[0].PreviousSize, 0.f, 0.f );

    // Moved but not resized: nothing to report.
    Clear( changes );
    root.Padding( Edges::All( 4_u ) );
    plain.FixedHeight( 50_u );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE( Empty( changes ) );

    watched.FixedHeight( 45_u );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE( Size( changes ) == 1 );
    REQUIRE_VEC2( changes[0].PreviousSize, 30.f, 30.f );
}

TEST_CASE( "ContentSize includes what overflows a child, unless the child clips it", "[compose]" )
{
    LayoutFixture fx;
    LayoutNode    root, panel, tall;
    root.LayoutType( ELayoutType::Vertical );
    panel.FixedWidth( 100_u ).FixedHeight( 50_u );
    tall.FixedWidth( 40_u ).FixedHeight( 180_u );
    root.PushBackChild( panel );
    panel.PushBackChild( tall );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_VEC2( panel.Layout.ContentSize, 40.f, 180.f );
    REQUIRE_VEC2( root.Layout.ContentSize, 100.f, 180.f );

    panel.ClipsContent = true;
    root.MarkDirty();
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );
    REQUIRE_VEC2( root.Layout.ContentSize, 100.f, 50.f );
}

TEST_CASE( "ContentSize of a node without children is its padding", "[compose]" )
{
    LayoutFixture fx;
    LayoutNode    leaf;
    leaf.Padding( Edges{ .T = 1_u, .R = 2_u, .B = 3_u, .L = 4_u } );
    RunLayout( leaf, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_VEC2( leaf.Layout.ContentSize, 6.f, 4.f );
}

TEST_CASE( "ContentSize includes the children's trailing margins and Hidden children", "[compose]" )
{
    LayoutFixture fx;
    LayoutNode    root, a, b;
    root.LayoutType( ELayoutType::Vertical );
    a.FixedWidth( 20_u ).FixedHeight( 20_u ).Margin( Edges{ .R = 7_u, .B = 10_u } );
    b.FixedWidth( 20_u ).FixedHeight( 20_u ).Visibility( EVisibility::Hidden );
    root.PushBackChild( a );
    root.PushBackChild( b );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_VEC2( root.Layout.ContentSize, 27.f, 50.f );
}

TEST_CASE( "ContentSize includes the safe area a node keeps clear of", "[compose]" )
{
    LayoutFixture fx;
    fx.Ctx.RemainingSafeArea = Edges{ .B = 30_u };

    LayoutNode root, child;
    root.LayoutType( ELayoutType::Vertical ).SafeAreaEdges( ESafeAreaEdges::Bottom );
    child.FixedWidth( 20_u ).FixedHeight( 20_u );
    root.PushBackChild( child );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE_VEC2( root.Layout.ContentSize, 20.f, 50.f );
}

TEST_CASE( "Arrange reports a content size change even when the node's own size stays", "[compose]" )
{
    LayoutFixture         fx;
    Array<GeometryChange> changes;
    fx.Ctx.GeometryChanges = &changes;

    LayoutNode root, watched, child;
    root.LayoutType( ELayoutType::Vertical );
    watched.FixedWidth( 30_u ).FixedHeight( 30_u );
    watched.NotifyGeometry = true;
    child.FixedWidth( 10_u ).FixedHeight( 10_u );
    root.PushBackChild( watched );
    watched.PushBackChild( child );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    Clear( changes );
    child.FixedHeight( 50_u );
    RunLayout( root, Vec2<Unit>{ 200_u, 200_u }, fx );

    REQUIRE( Size( changes ) == 1 );
    REQUIRE_VEC2( changes[0].PreviousSize, 30.f, 30.f );
    REQUIRE_VEC2( changes[0].PreviousContentSize, 10.f, 10.f );
    REQUIRE_VEC2( watched.Layout.ContentSize, 10.f, 50.f );
}

TEST_CASE( "SetContentOffset reports whether the offset changed", "[compose]" )
{
    NestedFixture f;

    REQUIRE_FALSE( SetContentOffset( f.Root, Vec2<Unit>{ 0_u, 0_u } ) );
    REQUIRE( SetContentOffset( f.Root, Vec2<Unit>{ 0_u, 5_u } ) );
    REQUIRE_FALSE( SetContentOffset( f.Root, Vec2<Unit>{ 0_u, 5_u } ) );
    REQUIRE_RECT( f.Leaf.Layout.SceneRect, 15.f, 20.f, 20.f, 20.f );
}
