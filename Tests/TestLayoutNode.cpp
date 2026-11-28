/**
 * @file TestLayoutNode.cpp
 * @brief Tests for LayoutNode's tree structure (attach/detach/insert), dirty-flag propagation,
 *        EVisibility::Apply, and the Edges/Constraints style utility types.
 */

#include "TestLayoutCommon.h"

// =============================================================================
// Hierarchy: attach / detach / insert
// =============================================================================

TEST_CASE( "PushBackChild attaches a child at the end", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushBackChild( b );

    REQUIRE( parent.ChildCount() == 2 );
    REQUIRE( parent.FirstChild() == &a );
    REQUIRE( parent.LastChild()  == &b );
    REQUIRE( a.Parent() == &parent );
    REQUIRE( a.NextSibling() == &b );
    REQUIRE( b.PrevSibling() == &a );
    REQUIRE( b.NextSibling() == nullptr );
}

TEST_CASE( "PushFrontChild attaches a child at the start", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.PushFrontChild( b );

    REQUIRE( parent.ChildCount() == 2 );
    REQUIRE( parent.FirstChild() == &b );
    REQUIRE( parent.LastChild()  == &a );
    REQUIRE( b.NextSibling() == &a );
    REQUIRE( a.PrevSibling() == &b );
}

TEST_CASE( "InsertChildAfter inserts between two existing siblings", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( c );
    parent.InsertChildAfter( b, a );

    REQUIRE( parent.ChildCount() == 3 );
    REQUIRE( a.NextSibling() == &b );
    REQUIRE( b.PrevSibling() == &a );
    REQUIRE( b.NextSibling() == &c );
    REQUIRE( c.PrevSibling() == &b );
}

TEST_CASE( "InsertChildAfter on the current last child updates LastChild", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.InsertChildAfter( b, a );

    REQUIRE( parent.LastChild() == &b );
    REQUIRE( b.NextSibling() == nullptr );
}

TEST_CASE( "InsertChildBefore inserts between two existing siblings", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( c );
    parent.InsertChildBefore( b, c );

    REQUIRE( parent.ChildCount() == 3 );
    REQUIRE( a.NextSibling() == &b );
    REQUIRE( b.PrevSibling() == &a );
    REQUIRE( b.NextSibling() == &c );
    REQUIRE( c.PrevSibling() == &b );
}

TEST_CASE( "InsertChildBefore on the current first child updates FirstChild", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b;
    parent.PushBackChild( a );
    parent.InsertChildBefore( b, a );

    REQUIRE( parent.FirstChild() == &b );
    REQUIRE( b.PrevSibling() == nullptr );
}

TEST_CASE( "DetachFromParent removes a middle child and relinks its siblings", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );

    b.DetachFromParent();

    REQUIRE( parent.ChildCount() == 2 );
    REQUIRE( a.NextSibling() == &c );
    REQUIRE( c.PrevSibling() == &a );
    REQUIRE( b.Parent() == nullptr );
    REQUIRE( b.NextSibling() == nullptr );
    REQUIRE( b.PrevSibling() == nullptr );
}

TEST_CASE( "DetachFromParent on an only child clears FirstChild and LastChild", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a;
    parent.PushBackChild( a );
    a.DetachFromParent();

    REQUIRE( parent.ChildCount() == 0 );
    REQUIRE( parent.FirstChild() == nullptr );
    REQUIRE( parent.LastChild()  == nullptr );
}

TEST_CASE( "DetachFromParent on a node with no parent is a harmless no-op", "[layout-node][hierarchy]" )
{
    LayoutNode orphan;
    orphan.DetachFromParent();

    REQUIRE( orphan.Parent() == nullptr );
}

TEST_CASE( "Re-parenting an attached child moves it instead of duplicating it", "[layout-node][hierarchy]" )
{
    LayoutNode parentA, parentB, child;
    parentA.PushBackChild( child );
    parentB.PushBackChild( child );

    REQUIRE( parentA.ChildCount() == 0 );
    REQUIRE( parentB.ChildCount() == 1 );
    REQUIRE( child.Parent() == &parentB );
}

TEST_CASE( "ForEachChild visits children in forward order", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );

    std::vector<LayoutNode*> visited;
    parent.ForEachChild( [&]( LayoutNode& node ) { visited.push_back( &node ); } );

    REQUIRE( visited == std::vector<LayoutNode*>{ &a, &b, &c } );
}

TEST_CASE( "ForEachChildReverse visits children in reverse order", "[layout-node][hierarchy]" )
{
    LayoutNode parent, a, b, c;
    parent.PushBackChild( a );
    parent.PushBackChild( b );
    parent.PushBackChild( c );

    std::vector<LayoutNode*> visited;
    parent.ForEachChildReverse( [&]( LayoutNode& node ) { visited.push_back( &node ); } );

    REQUIRE( visited == std::vector<LayoutNode*>{ &c, &b, &a } );
}

TEST_CASE( "ForEachDescendant visits the full subtree depth-first", "[layout-node][hierarchy]" )
{
    LayoutNode root, a, b, aa;
    root.PushBackChild( a );
    root.PushBackChild( b );
    a.PushBackChild( aa );

    std::vector<LayoutNode*> visited;
    root.ForEachDescendant( [&]( LayoutNode& node ) { visited.push_back( &node ); } );

    REQUIRE( visited == std::vector<LayoutNode*>{ &a, &aa, &b } );
}

// =============================================================================
// Dirty-flag propagation
// =============================================================================

TEST_CASE( "MarkDirty sets IsDirty on the node itself", "[layout-node][dirty]" )
{
    LayoutNode node;
    node.Layout.IsDirty = false;
    node.MarkDirty();

    REQUIRE( node.Layout.IsDirty );
}

TEST_CASE( "MarkDirty propagates IsDescendantDirty up through every ancestor", "[layout-node][dirty]" )
{
    LayoutNode root, mid, leaf;
    root.PushBackChild( mid );
    mid.PushBackChild( leaf );
    root.Layout.IsDescendantDirty = false;
    mid.Layout.IsDescendantDirty  = false;
    leaf.Layout.IsDescendantDirty = false;
    
    leaf.MarkDirty();

    REQUIRE( leaf.Layout.IsDirty );
    REQUIRE( mid.Layout.IsDescendantDirty );
    REQUIRE( root.Layout.IsDescendantDirty );
    // MarkDirty only marks the node itself dirty, never its own descendant flag.
    REQUIRE_FALSE( leaf.Layout.IsDescendantDirty );
}

TEST_CASE( "MarkDirty on a node does not touch any of its children's flags", "[layout-node][dirty]" )
{
    LayoutNode root, child;
    root.PushBackChild( child );
    child.Layout.IsDirty = false;

    root.MarkDirty();

    REQUIRE( root.Layout.IsDirty );
    REQUIRE_FALSE( child.Layout.IsDirty );
}

TEST_CASE( "MarkDescendantDirty stops climbing once it reaches an already-marked ancestor", "[layout-node][dirty]" )
{
    LayoutNode root, mid, leaf;
    root.PushBackChild( mid );
    mid.PushBackChild( leaf );

    root.Layout.IsDescendantDirty = false;
    mid.Layout.IsDescendantDirty  = true; // Pretend an earlier change already marked this one.
    leaf.Layout.IsDescendantDirty = false;

    leaf.MarkDescendantDirty();

    REQUIRE( leaf.Layout.IsDescendantDirty );
    // mid was already marked, so the walk stops there and root is left untouched.
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );
}

TEST_CASE( "Dirtying two siblings still reaches their shared ancestor correctly", "[layout-node][dirty]" )
{
    LayoutNode root, a, b;
    root.PushBackChild( a );
    root.PushBackChild( b );
    root.Layout.IsDescendantDirty = false;

    a.MarkDirty();
    b.MarkDirty(); // Should short-circuit cleanly since root is already marked; must not misbehave.

    REQUIRE( root.Layout.IsDescendantDirty );
    REQUIRE( a.Layout.IsDirty );
    REQUIRE( b.Layout.IsDirty );
}

// =============================================================================
// EVisibility::Apply
// =============================================================================

TEST_CASE( "Visibility::Apply gives a child the more hidden of its own and its parent's visibility", "[layout-node][visibility]" )
{
    REQUIRE( Visibility::Apply( EVisibility::Visible, EVisibility::Visible ) == EVisibility::Visible );
    REQUIRE( Visibility::Apply( EVisibility::Hidden, EVisibility::Visible ) == EVisibility::Hidden );
    REQUIRE( Visibility::Apply( EVisibility::Visible, EVisibility::Hidden ) == EVisibility::Hidden );
    REQUIRE( Visibility::Apply( EVisibility::Collapsed, EVisibility::Visible ) == EVisibility::Collapsed );
    REQUIRE( Visibility::Apply( EVisibility::Hidden, EVisibility::Collapsed ) == EVisibility::Collapsed );
}

TEST_CASE( "Hidden takes space but isn't drawn; Collapsed does neither", "[layout-node][visibility]" )
{
    REQUIRE( Visibility::AffectsLayout( EVisibility::Hidden ) );
    REQUIRE_FALSE( Visibility::IsRendered( EVisibility::Hidden ) );
    REQUIRE_FALSE( Visibility::AffectsLayout( EVisibility::Collapsed ) );
    REQUIRE_FALSE( Visibility::IsRendered( EVisibility::Collapsed ) );
}

TEST_CASE( "Hit testing is switched off for a node and everything below it, apart from visibility", "[layout-node][visibility]" )
{
    LayoutNode root, child;
    root.PushBackChild( child );

    root.HitTestable( false );
    REQUIRE( root.Layout.Visibility == EVisibility::Visible );
    REQUIRE_FALSE( root.Layout.HitTestable );
    REQUIRE_FALSE( child.Layout.HitTestable );

    // Showing a node doesn't change how it takes input.
    root.Visibility( EVisibility::Hidden );
    root.Visibility( EVisibility::Visible );
    REQUIRE_FALSE( child.Layout.HitTestable );

    root.HitTestable( true );
    REQUIRE( child.Layout.HitTestable );
}

TEST_CASE( "A node that isn't drawn can't be hit", "[layout-node][visibility]" )
{
    LayoutNode root, child;
    root.PushBackChild( child );

    root.Visibility( EVisibility::Hidden );
    REQUIRE_FALSE( root.Layout.HitTestable );
    REQUIRE_FALSE( child.Layout.HitTestable );
    REQUIRE( child.Layout.Visibility == EVisibility::Hidden );
}

TEST_CASE( "Only entering or leaving Collapsed marks layout dirty", "[layout-node][visibility]" )
{
    LayoutNode root, child;
    root.PushBackChild( child );
    root.Layout.IsDirty = root.Layout.IsDescendantDirty = false;
    child.Layout.IsDirty = child.Layout.IsDescendantDirty = false;

    child.Visibility( EVisibility::Hidden );
    child.HitTestable( false );
    REQUIRE_FALSE( child.Layout.IsDirty );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );

    child.Visibility( EVisibility::Collapsed );
    REQUIRE( child.Layout.IsDirty );
    REQUIRE( root.Layout.IsDescendantDirty );
}

// =============================================================================
// Edges
// =============================================================================

TEST_CASE( "Edges::All sets every side to the same value", "[layout-node][edges]" )
{
    Edges e = Edges::All( 10_u );

    REQUIRE_UNIT( e.T, 10.f );
    REQUIRE_UNIT( e.R, 10.f );
    REQUIRE_UNIT( e.B, 10.f );
    REQUIRE_UNIT( e.L, 10.f );
}

TEST_CASE( "Edges::Axis sets horizontal and vertical sides independently", "[layout-node][edges]" )
{
    Edges e = Edges::Axis( 5_u, 8_u );

    REQUIRE_UNIT( e.L, 5.f );
    REQUIRE_UNIT( e.R, 5.f );
    REQUIRE_UNIT( e.T, 8.f );
    REQUIRE_UNIT( e.B, 8.f );
}

TEST_CASE( "Edges::Horizontal and Vertical sum the correct opposing sides", "[layout-node][edges]" )
{
    Edges e{ .T = 1_u, .R = 2_u, .B = 3_u, .L = 4_u };

    REQUIRE_UNIT( e.Horizontal(), 6.f ); // L + R
    REQUIRE_UNIT( e.Vertical(),   4.f ); // T + B
}

TEST_CASE( "Edges::Apply insets a rect by the corresponding side on each edge", "[layout-node][edges]" )
{
    Edges e{ .T = 5_u, .R = 10_u, .B = 15_u, .L = 20_u };
    Rect<Unit> rect{ Vec2<Unit>{ 0_u, 0_u }, Vec2<Unit>{ 100_u, 100_u } };

    Rect<Unit> inset = e.Apply( rect );

    REQUIRE_RECT( inset, 20.f, 5.f, 70.f, 80.f ); // width: 100-20-10, height: 100-5-15
}

// =============================================================================
// Constraints
// =============================================================================

TEST_CASE( "Constraints::AtLeast only bounds the minimum, leaving the maximum unbounded", "[layout-node][constraints]" )
{
    Constraints c = Constraints::AtLeast( Vec2<Unit>{ 20_u, 30_u } );

    REQUIRE_UNIT( c.Min[0], 20.f );
    REQUIRE_UNIT( c.Min[1], 30.f );
    REQUIRE( c.Max[0] == Limits<Unit>::max() );
    REQUIRE( c.Max[1] == Limits<Unit>::max() );
}

TEST_CASE( "Constraints::AtMost only bounds the maximum, leaving the minimum at zero", "[layout-node][constraints]" )
{
    Constraints c = Constraints::AtMost( Vec2<Unit>{ 200_u, 300_u } );

    REQUIRE_UNIT( c.Max[0], 200.f );
    REQUIRE_UNIT( c.Max[1], 300.f );
    REQUIRE( c.Min[0] == 0_u );
    REQUIRE( c.Min[1] == 0_u );
}

TEST_CASE( "Constraints::Fixed pins both the minimum and maximum to the same size", "[layout-node][constraints]" )
{
    Constraints c = Constraints::Fixed( Vec2<Unit>{ 50_u, 60_u } );

    REQUIRE( c.Min == c.Max );
    REQUIRE_UNIT( c.Min[0], 50.f );
    REQUIRE_UNIT( c.Min[1], 60.f );
}

TEST_CASE( "Setting the visibility or hit-testability a node already has changes nothing", "[layout-node]" )
{
    LayoutNode root, child;
    root.PushBackChild( child );
    root.Layout.IsHitTestDirty = false;

    child.Visibility( EVisibility::Visible ).HitTestable( true );
    REQUIRE_FALSE( root.Layout.IsHitTestDirty );

    child.Visibility( EVisibility::Hidden );
    REQUIRE( root.Layout.IsHitTestDirty );
}
