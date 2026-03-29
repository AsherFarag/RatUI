/**
 * @file TestLayoutNode.cpp
 * @brief Tests for RatUI::LayoutNode and RatUI::MeasureLayoutNode.
 */

#include "../Common/Common.h"

using namespace RatUI;

// =============================================================================
// MeasureLayoutNode – sizing modes (no children)
// =============================================================================

TEST_CASE( "MeasureLayoutNode Fixed width and height returns exactly FixedWidth/FixedHeight", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = 120.0f;
    w.Style.FixedHeight = 80.0f;

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 120.0f, 80.0f ) );
    RequireApproxEqual( w.Layout.DesiredSize, Vec2f( 120.0f, 80.0f ) );
}

//TEST_CASE( "MeasureLayoutNode Fill width and height uses available size multiplied by percent", "[LayoutNode][measure]" )
//{
//    LayoutNode w{};
//    w.Style.WidthMode    = ESizingMode::Fill;
//    w.Style.HeightMode   = ESizingMode::Fill;
//    w.Style.PercentWidth  = 0.5f;
//    w.Style.PercentHeight = 0.25f;
//
//    Vec2f result = MeasureLayoutNode( w, Vec2f( 400.0f, 200.0f ) );
//    RequireApproxEqual( result, Vec2f( 200.0f, 50.0f ) );
//}

TEST_CASE( "MeasureLayoutNode Content mode with no children returns zero size", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode  = ESizingMode::Content;
    w.Style.HeightMode = ESizingMode::Content;

    Vec2f result = MeasureLayoutNode( w, Vec2f( 500.0f, 500.0f ) );
    RequireApproxEqual( result, Vec2f( 0.0f, 0.0f ) );
}

TEST_CASE( "MeasureLayoutNode Content mode with no children but padding returns padding total", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode  = ESizingMode::Content;
    w.Style.HeightMode = ESizingMode::Content;
    w.Style.Padding    = Edges::Symmetric( 10.0f, 5.0f ); // H=10 each side, V=5 each side

    Vec2f result = MeasureLayoutNode( w, Vec2f( 500.0f, 500.0f ) );
    RequireApproxEqual( result, Vec2f( 20.0f, 10.0f ) );
}

// =============================================================================
// MeasureLayoutNode – DesiredSize is stored on the LayoutNode
// =============================================================================

TEST_CASE( "MeasureLayoutNode stores the result in LayoutNode::Layout::DesiredSize", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = 64.0f;
    w.Style.FixedHeight = 32.0f;

    MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( w.Layout.DesiredSize, Vec2f( 64.0f, 32.0f ) );
}

// =============================================================================
// MeasureLayoutNode – SizeConstraints clamping
// =============================================================================

TEST_CASE( "MeasureLayoutNode clamps Fixed size to SizeConstraints min", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = 10.0f;
    w.Style.FixedHeight = 10.0f;
    w.Style.SizeConstraints = Constraints::AtLeast( Vec2f( 50.0f, 50.0f ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 50.0f, 50.0f ) );
}

TEST_CASE( "MeasureLayoutNode clamps Fixed size to SizeConstraints max", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = 500.0f;
    w.Style.FixedHeight = 500.0f;
    w.Style.SizeConstraints = Constraints::AtMost( Vec2f( 100.0f, 100.0f ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 100.0f, 100.0f ) );
}

TEST_CASE( "MeasureLayoutNode Fixed constraints keep size unchanged when within bounds", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = 80.0f;
    w.Style.FixedHeight = 60.0f;
    w.Style.SizeConstraints = Constraints::Fixed( Vec2f( 80.0f, 60.0f ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 80.0f, 60.0f ) );
}

// =============================================================================
// MeasureLayoutNode – Horizontal layout children
// =============================================================================

TEST_CASE( "MeasureLayoutNode Horizontal layout sums child widths", "[LayoutNode][measure][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 30.0f; c1.Style.FixedHeight = 20.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 50.0f; c2.Style.FixedHeight = 10.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = 30 + 50 (no spacing), Height = max(20, 10) = 20
    RequireApproxEqual( result, Vec2f( 80.0f, 20.0f ) );
}

TEST_CASE( "MeasureLayoutNode Horizontal layout adds spacing between children", "[LayoutNode][measure][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Spacing    = 10.0f;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 40.0f; c1.Style.FixedHeight = 20.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 60.0f; c2.Style.FixedHeight = 15.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = 40 + 10 (spacing) + 60 = 110, Height = max(20,15) = 20
    RequireApproxEqual( result, Vec2f( 110.0f, 20.0f ) );
}

TEST_CASE( "MeasureLayoutNode Horizontal layout with three children and spacing", "[LayoutNode][measure][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Spacing    = 5.0f;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 10.0f; c1.Style.FixedHeight = 10.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 20.0f; c2.Style.FixedHeight = 30.0f;

    LayoutNode c3{}; c3.Style.WidthMode = ESizingMode::Fixed; c3.Style.HeightMode = ESizingMode::Fixed;
    c3.Style.FixedWidth = 15.0f; c3.Style.FixedHeight = 20.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );
    parent.PushBackChild( c3 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = 10 + 5 + 20 + 5 + 15 = 55 (2 gaps for 3 children), Height = max(10,30,20) = 30
    RequireApproxEqual( result, Vec2f( 55.0f, 30.0f ) );
}

// =============================================================================
// MeasureLayoutNode – Vertical layout children
// =============================================================================

TEST_CASE( "MeasureLayoutNode Vertical layout sums child heights", "[LayoutNode][measure][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 40.0f; c1.Style.FixedHeight = 20.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 60.0f; c2.Style.FixedHeight = 30.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = max(40, 60) = 60, Height = 20 + 30 = 50
    RequireApproxEqual( result, Vec2f( 60.0f, 50.0f ) );
}

TEST_CASE( "MeasureLayoutNode Vertical layout adds spacing between children", "[LayoutNode][measure][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Spacing    = 8.0f;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 50.0f; c1.Style.FixedHeight = 20.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 30.0f; c2.Style.FixedHeight = 40.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = max(50, 30) = 50, Height = 20 + 8 + 40 = 68
    RequireApproxEqual( result, Vec2f( 50.0f, 68.0f ) );
}

// =============================================================================
// MeasureLayoutNode – Overlay layout children
// =============================================================================

TEST_CASE( "MeasureLayoutNode Overlay layout uses max of child sizes", "[LayoutNode][measure][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 100.0f; c1.Style.FixedHeight = 50.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = 60.0f; c2.Style.FixedHeight = 80.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = max(100, 60) = 100, Height = max(50, 80) = 80
    RequireApproxEqual( result, Vec2f( 100.0f, 80.0f ) );
}

// =============================================================================
// MeasureLayoutNode – padding
// =============================================================================

TEST_CASE( "MeasureLayoutNode Horizontal layout adds padding around content", "[LayoutNode][measure][padding]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Padding    = Edges::Uniform( 10.0f );

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 50.0f; c1.Style.FixedHeight = 30.0f;

    parent.PushBackChild( c1 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = 50 + 10 (left) + 10 (right) = 70, Height = 30 + 10 (top) + 10 (bottom) = 50
    RequireApproxEqual( result, Vec2f( 70.0f, 50.0f ) );
}

TEST_CASE( "MeasureLayoutNode Vertical layout adds asymmetric padding around content", "[LayoutNode][measure][padding]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Padding    = Edges::Asymmetric( 4.0f, 8.0f, 4.0f, 8.0f ); // top=4, right=8, bottom=4, left=8

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = 40.0f; c1.Style.FixedHeight = 20.0f;

    parent.PushBackChild( c1 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Width = 40 + 8 + 8 = 56, Height = 20 + 4 + 4 = 28
    RequireApproxEqual( result, Vec2f( 56.0f, 28.0f ) );
}

// =============================================================================
// MeasureLayoutNode – recursive measurement
// =============================================================================

TEST_CASE( "MeasureLayoutNode recursively measures child LayoutNodes", "[LayoutNode][measure]" )
{
    // Grandchild is Content-sized with no children -> 0x0
    // Child is Fixed 50x30
    // Parent accumulates child in Horizontal layout

    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode child{};
    child.Style.WidthMode  = ESizingMode::Content;
    child.Style.HeightMode = ESizingMode::Content;
    child.Style.LayoutType = ELayoutType::Overlay;

    LayoutNode grandchild{};
    grandchild.Style.WidthMode   = ESizingMode::Fixed;
    grandchild.Style.HeightMode  = ESizingMode::Fixed;
    grandchild.Style.FixedWidth  = 70.0f;
    grandchild.Style.FixedHeight = 40.0f;

    child.PushBackChild( grandchild );
    parent.PushBackChild( child );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 70.0f, 40.0f ) );
}

TEST_CASE( "MeasureLayoutNode child DesiredSize is populated during parent measurement", "[LayoutNode][measure]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode child{};
    child.Style.WidthMode   = ESizingMode::Fixed;
    child.Style.HeightMode  = ESizingMode::Fixed;
    child.Style.FixedWidth  = 55.0f;
    child.Style.FixedHeight = 35.0f;

    parent.PushBackChild( child );

    MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );

    RequireApproxEqual( child.Layout.DesiredSize, Vec2f( 55.0f, 35.0f ) );
}
//
//TEST_CASE( "MeasureLayoutNode child with Fill mode in parent with Content mode falls back to Content for measurement", "[LayoutNode][measure]" )
//{
//    LayoutNode parent{};
//    parent.Style.LayoutType = ELayoutType::Horizontal;
//    parent.Style.WidthMode  = ESizingMode::Content;
//    parent.Style.HeightMode = ESizingMode::Content;
//
//    LayoutNode child{};
//    child.Style.WidthMode   = ESizingMode::Fill; // Circular dependency if treated as Fill
//    child.Style.HeightMode  = ESizingMode::Fill; // Circular dependency if treated as Fill
//    child.Style.PercentWidth = 1.0f; // Would want to take all available width, but should fall back to Content
//    child.Style.PercentHeight = 1.0f; // Would want to take all available height, but should fall back to Content
//    parent.PushBackChild( child );
//
//    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
//
//    // If the child's Fill is treated as Content during measure, it resolves to 0x0, so the parent's size is also 0x0.
//    RequireApproxEqual( result, Vec2f( 0.0f, 0.0f ) );
//
//	// The child's DesiredSize should also be 0x0 since it falls back to Content mode during measurement.
//    RequireApproxEqual( child.Layout.DesiredSize, Vec2f( 0.0f, 0.0f ) );
//}

//TEST_CASE( "ArrangeLinear expands Fill child after Content parent is resolved", "[LayoutNode][arrange]" )
//{
//    LayoutNode parent{};
//    parent.Style.LayoutType = ELayoutType::Horizontal;
//    parent.Style.WidthMode = ESizingMode::Fixed;
//    parent.Style.FixedWidth = 400.f;
//    parent.Style.HeightMode = ESizingMode::Fixed;
//    parent.Style.FixedHeight = 200.f;
//
//    LayoutNode child{};
//    child.Style.WidthMode = ESizingMode::Fill;
//    child.Style.HeightMode = ESizingMode::Fill;
//    child.Style.PercentWidth = 1.0f;
//    child.Style.PercentHeight = 1.0f;
//    parent.PushBackChild( child );
//
//    MeasureLayoutNode( parent, Vec2f( 400.f, 200.f ) );
//    ArrangeLayoutNode( parent, Rectf{ .Origin = { 0.f, 0.f }, .Size = { 400.f, 200.f } } );
//
//    RequireApproxEqual( child.Layout.FinalRect.Size, Vec2f( 400.f, 200.f ) );
//    RequireApproxEqual( child.Layout.FinalRect.Origin, Vec2f( 0.f, 0.f ) );
//}