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
    w.Style.FixedWidth  = Unit{ 120.0f };
    w.Style.FixedHeight = Unit{ 80.0f };

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
    w.Style.Padding    = Edges::Symmetric( Unit{ 10.0f }, Unit{ 5.0f } ); // H=10 each side, V=5 each side

    Vec2f result = MeasureLayoutNode( w, Vec2f( 500.0f, 500.0f ) );
    RequireApproxEqual( result, Vec2f( 20.0f, 10.0f ) );
}

// =============================================================================
// MeasureLayoutNode – DesiredSize is stored on the LayoutNode
// =============================================================================

// (covered by every sizing-mode test above; no separate redundant test needed)

// =============================================================================
// MeasureLayoutNode – SizeConstraints clamping
// =============================================================================

TEST_CASE( "MeasureLayoutNode clamps Fixed size to SizeConstraints min", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = Unit{ 10.0f };
    w.Style.FixedHeight = Unit{ 10.0f };
    w.Style.SizeConstraints = Constraints::AtLeast( ToUnitVec2( Vec2f( 50.0f, 50.0f ) ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 50.0f, 50.0f ) );
}

TEST_CASE( "MeasureLayoutNode clamps Fixed size to SizeConstraints max", "[LayoutNode][measure]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = Unit{ 500.0f };
    w.Style.FixedHeight = Unit{ 500.0f };
    w.Style.SizeConstraints = Constraints::AtMost( ToUnitVec2( Vec2f( 100.0f, 100.0f ) ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 100.0f, 100.0f ) );
}

TEST_CASE( "MeasureLayoutNode Fixed constraints keep size unchanged when within bounds", "[LayoutNode][measure]" )
{
    // (clamp(x,x,x)==x; validates Constraints::Fixed round-trips cleanly)
    LayoutNode w{};
    w.Style.WidthMode   = ESizingMode::Fixed;
    w.Style.HeightMode  = ESizingMode::Fixed;
    w.Style.FixedWidth  = Unit{ 80.0f };
    w.Style.FixedHeight = Unit{ 60.0f };
    w.Style.SizeConstraints = Constraints::Fixed( ToUnitVec2( Vec2f( 80.0f, 60.0f ) ) );

    Vec2f result = MeasureLayoutNode( w, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 80.0f, 60.0f ) );
}

// =============================================================================
// MeasureLayoutNode – Percent sizing mode
// =============================================================================

TEST_CASE( "MeasureLayoutNode Percent width and height scales with available size", "[LayoutNode][measure][percent]" )
{
    LayoutNode w{};
    w.Style.WidthMode     = ESizingMode::Percent;
    w.Style.HeightMode    = ESizingMode::Percent;
    w.Style.PercentWidth  = 0.5f;
    w.Style.PercentHeight = 0.25f;

    Vec2f result = MeasureLayoutNode( w, Vec2f( 400.0f, 200.0f ) );
    RequireApproxEqual( result, Vec2f( 200.0f, 50.0f ) );
    RequireApproxEqual( w.Layout.DesiredSize, Vec2f( 200.0f, 50.0f ) );
}

TEST_CASE( "MeasureLayoutNode mixed sizing modes: Fixed width, Percent height", "[LayoutNode][measure][percent]" )
{
    LayoutNode w{};
    w.Style.WidthMode     = ESizingMode::Fixed;
    w.Style.HeightMode    = ESizingMode::Percent;
    w.Style.FixedWidth    = Unit{ 80.0f };
    w.Style.PercentHeight = 0.5f;

    Vec2f result = MeasureLayoutNode( w, Vec2f( 200.0f, 100.0f ) );
    RequireApproxEqual( result, Vec2f( 80.0f, 50.0f ) );
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
    c1.Style.FixedWidth = Unit{ 30.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 50.0f }; c2.Style.FixedHeight = Unit{ 10.0f };

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
    parent.Style.Spacing    = Unit{ 10.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 40.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 15.0f };

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
    parent.Style.Spacing    = Unit{ 5.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 10.0f }; c1.Style.FixedHeight = Unit{ 10.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 20.0f }; c2.Style.FixedHeight = Unit{ 30.0f };

    LayoutNode c3{}; c3.Style.WidthMode = ESizingMode::Fixed; c3.Style.HeightMode = ESizingMode::Fixed;
    c3.Style.FixedWidth = Unit{ 15.0f }; c3.Style.FixedHeight = Unit{ 20.0f };

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
    c1.Style.FixedWidth = Unit{ 40.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 30.0f };

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
    parent.Style.Spacing    = Unit{ 8.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 30.0f }; c2.Style.FixedHeight = Unit{ 40.0f };

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
    c1.Style.FixedWidth = Unit{ 100.0f }; c1.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 80.0f };

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
    parent.Style.Padding    = Edges::Uniform( Unit{ 10.0f } );

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

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
    parent.Style.Padding    = Edges::Asymmetric( Unit{ 4.0f }, Unit{ 8.0f }, Unit{ 4.0f }, Unit{ 8.0f } ); // top=4, right=8, bottom=4, left=8

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 40.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

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
    grandchild.Style.FixedWidth  = Unit{ 70.0f };
    grandchild.Style.FixedHeight = Unit{ 40.0f };

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
    child.Style.FixedWidth  = Unit{ 55.0f };
    child.Style.FixedHeight = Unit{ 35.0f };

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

// =============================================================================
// MeasureLayoutNode – single child with spacing (trailing spacing bug regression)
// =============================================================================

TEST_CASE( "MeasureLayoutNode Horizontal single child with spacing has no trailing spacing added", "[LayoutNode][measure][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Spacing    = Unit{ 10.0f };

    LayoutNode child{}; child.Style.WidthMode = ESizingMode::Fixed; child.Style.HeightMode = ESizingMode::Fixed;
    child.Style.FixedWidth = Unit{ 50.0f }; child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Only 1 child → no spacing gap; spacing should NOT be added
    RequireApproxEqual( result, Vec2f( 50.0f, 20.0f ) );
}

TEST_CASE( "MeasureLayoutNode Vertical single child with spacing has no trailing spacing added", "[LayoutNode][measure][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;
    parent.Style.Spacing    = Unit{ 15.0f };

    LayoutNode child{}; child.Style.WidthMode = ESizingMode::Fixed; child.Style.HeightMode = ESizingMode::Fixed;
    child.Style.FixedWidth = Unit{ 40.0f }; child.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( child );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 40.0f, 30.0f ) );
}

// =============================================================================
// MeasureLayoutNode – Collapsed children
// =============================================================================

TEST_CASE( "MeasureLayoutNode Collapsed child contributes nothing to Content parent size", "[LayoutNode][measure][visibility]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode visible{}; visible.Style.WidthMode = ESizingMode::Fixed; visible.Style.HeightMode = ESizingMode::Fixed;
    visible.Style.FixedWidth = Unit{ 60.0f }; visible.Style.FixedHeight = Unit{ 40.0f };

    LayoutNode collapsed{}; collapsed.Style.WidthMode = ESizingMode::Fixed; collapsed.Style.HeightMode = ESizingMode::Fixed;
    collapsed.Style.FixedWidth = Unit{ 200.0f }; collapsed.Style.FixedHeight = Unit{ 200.0f };
    collapsed.Style.Visibility = EVisibility::Collapsed;

    parent.PushBackChild( visible );
    parent.PushBackChild( collapsed );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Only the visible child contributes; collapsed child is ignored
    RequireApproxEqual( result, Vec2f( 60.0f, 40.0f ) );
}

TEST_CASE( "MeasureLayoutNode all children Collapsed gives Content parent zero desired size", "[LayoutNode][measure][visibility]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizingMode::Content;
    parent.Style.HeightMode = ESizingMode::Content;

    LayoutNode c1{}; c1.Style.WidthMode = ESizingMode::Fixed; c1.Style.HeightMode = ESizingMode::Fixed;
    c1.Style.FixedWidth = Unit{ 100.0f }; c1.Style.FixedHeight = Unit{ 50.0f };
    c1.Style.Visibility = EVisibility::Collapsed;

    LayoutNode c2{}; c2.Style.WidthMode = ESizingMode::Fixed; c2.Style.HeightMode = ESizingMode::Fixed;
    c2.Style.FixedWidth = Unit{ 80.0f }; c2.Style.FixedHeight = Unit{ 60.0f };
    c2.Style.Visibility = EVisibility::Collapsed;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 0.0f, 0.0f ) );
}

// =============================================================================
// MeasureLayoutNode – SizeConstraints clamping on Content-mode node
// =============================================================================

TEST_CASE( "MeasureLayoutNode SizeConstraints clamps Content-mode desired size to max", "[LayoutNode][measure][constraints]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType     = ELayoutType::Horizontal;
    parent.Style.WidthMode      = ESizingMode::Content;
    parent.Style.HeightMode     = ESizingMode::Content;
    parent.Style.SizeConstraints = Constraints::AtMost( ToUnitVec2( Vec2f( 50.0f, 30.0f ) ) );

    LayoutNode child{}; child.Style.WidthMode = ESizingMode::Fixed; child.Style.HeightMode = ESizingMode::Fixed;
    child.Style.FixedWidth = Unit{ 200.0f }; child.Style.FixedHeight = Unit{ 100.0f };

    parent.PushBackChild( child );

    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    // Content would be 200×100 but max clamps it
    RequireApproxEqual( result, Vec2f( 50.0f, 30.0f ) );
}

TEST_CASE( "MeasureLayoutNode SizeConstraints clamps Content-mode desired size to min", "[LayoutNode][measure][constraints]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType     = ELayoutType::Overlay;
    parent.Style.WidthMode      = ESizingMode::Content;
    parent.Style.HeightMode     = ESizingMode::Content;
    parent.Style.SizeConstraints = Constraints::AtLeast( ToUnitVec2( Vec2f( 100.0f, 80.0f ) ) );

    // No children → content = 0×0, but min constraint forces 100×80
    Vec2f result = MeasureLayoutNode( parent, Vec2f( 1000.0f, 1000.0f ) );
    RequireApproxEqual( result, Vec2f( 100.0f, 80.0f ) );
}
