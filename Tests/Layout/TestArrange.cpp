/**
 * @file TestArrange.cpp
 * @brief Advanced tests for RatUI LayoutNode layout and arranging functions:
 *   AlignRect, AlignCrossAxis, ResolveAlign, ArrangeLayoutNode, ArrangeOverlay,
 *   ArrangeLinear (Horizontal / Vertical), ArrangeAnchored, and integrated
 *   Measure + Arrange round-trips with FlexGrow, margins, padding, visibility,
 *   SizeConstraints clamping, nested hierarchies, and edge-cases.
 */

#include "../Common/Common.h"

using namespace RatUI;

/** Runs MeasureLayoutNode then ArrangeLayoutNode on the root LayoutNode. */
static void DoLayout( LayoutNode& a_Root, Vec2f a_AvailSize )
{
    const Vec2<Unit> availSize = ToUnitVec2( a_AvailSize );
    MeasureLayoutNode( a_Root, availSize );
    ArrangeLayoutNode( a_Root, Rect<Unit>{ .Origin = { ToUnit( 0.0f ), ToUnit( 0.0f ) }, .Size = availSize } );
}

/** Checks Rectf origin and size. */
static void RequireRect( const Rectf& a_Rect, Vec2f a_Origin, Vec2f a_Size )
{
    REQUIRE( a_Rect.Origin[0] == Catch::Approx( a_Origin[0] ).epsilon( 1e-5f ) );
    REQUIRE( a_Rect.Origin[1] == Catch::Approx( a_Origin[1] ).epsilon( 1e-5f ) );
    REQUIRE( a_Rect.Size[0]   == Catch::Approx( a_Size[0]   ).epsilon( 1e-5f ) );
    REQUIRE( a_Rect.Size[1]   == Catch::Approx( a_Size[1]   ).epsilon( 1e-5f ) );
}

/** Checks Unit-rect origin and size against float expectations. */
static void RequireRect( const Rect<Unit>& a_Rect, Vec2f a_Origin, Vec2f a_Size )
{
    RequireRect( ToFloatRect( a_Rect ), a_Origin, a_Size );
}

static Anchor MakeAnchor( Vec2f a_Min, Vec2f a_Max, Vec2f a_Pivot, Vec2f a_Offset )
{
    Anchor anchor{};
    anchor.Min = a_Min;
    anchor.Max = a_Max;
    anchor.Pivot = a_Pivot;
    anchor.Offset = ToUnitVec2( a_Offset );
    return anchor;
}

static EAlign ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent )
{
    return a_Child.Style.SelfAlign != EAlign::Inherit
        ? a_Child.Style.SelfAlign
        : a_Parent.Style.ChildAlign;
}

// =============================================================================
// AlignRect
// =============================================================================

TEST_CASE( "AlignRect TopLeft places content at the container origin", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 100.0f, 50.0f }, .Size = { 200.0f, 100.0f } };
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::TopLeft );
    RequireRect( result, { 100.0f, 50.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect Center places content at the center of the container", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 0.0f, 0.0f }, .Size = { 200.0f, 100.0f } };
    // content 40x20 → offset (80, 40)
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::Center );
    RequireRect( result, { 80.0f, 40.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect BottomRight places content at the bottom-right of the container", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 0.0f, 0.0f }, .Size = { 200.0f, 100.0f } };
    // content 40x20 → offset (160, 80)
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::BottomRight );
    RequireRect( result, { 160.0f, 80.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect TopCenter places content horizontally centered at the top", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 0.0f, 0.0f }, .Size = { 200.0f, 100.0f } };
    // content 40x20 → offset x=(200-40)/2=80, y=0
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::TopCenter );
    RequireRect( result, { 80.0f, 0.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect CenterLeft places content vertically centered on the left", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 0.0f, 0.0f }, .Size = { 200.0f, 100.0f } };
    // content 40x20 → offset x=0, y=(100-20)/2=40
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::CenterLeft );
    RequireRect( result, { 0.0f, 40.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect BottomCenter places content horizontally centered at the bottom", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 0.0f, 0.0f }, .Size = { 200.0f, 100.0f } };
    Rectf result = AlignRect( { 40.0f, 20.0f }, container, EAlign::BottomCenter );
    RequireRect( result, { 80.0f, 80.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "AlignRect TopRight places content at the top-right corner", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 10.0f, 10.0f }, .Size = { 100.0f, 80.0f } };
    // content 20x15 → offset x = 10 + (100-20) = 90, y = 10
    Rectf result = AlignRect( { 20.0f, 15.0f }, container, EAlign::TopRight );
    RequireRect( result, { 90.0f, 10.0f }, { 20.0f, 15.0f } );
}

TEST_CASE( "AlignRect with non-zero container origin offsets result correctly", "[arrange][alignrect]" )
{
    Rectf container{ .Origin = { 50.0f, 30.0f }, .Size = { 100.0f, 60.0f } };
    Rectf result = AlignRect( { 100.0f, 60.0f }, container, EAlign::Center );
    // Content exactly fills container — origin should equal container origin
    RequireRect( result, { 50.0f, 30.0f }, { 100.0f, 60.0f } );
}

// =============================================================================
// AlignCrossAxis
// =============================================================================

TEST_CASE( "AlignCrossAxis TopLeft on horizontal layout places child at top", "[arrange][crossaxis]" )
{
    // Horizontal layout → cross axis is vertical → checking Top flag
    f32 pos = AlignCrossAxis( 20.0f, 0.0f, 100.0f, EAlign::TopLeft, /*isHz=*/true );
    REQUIRE( pos == Catch::Approx( 0.0f ) );
}

TEST_CASE( "AlignCrossAxis VCenter on horizontal layout centers child vertically", "[arrange][crossaxis]" )
{
    f32 pos = AlignCrossAxis( 20.0f, 0.0f, 100.0f, EAlign::Center, /*isHz=*/true );
    REQUIRE( pos == Catch::Approx( 40.0f ) ); // (100-20)/2
}

TEST_CASE( "AlignCrossAxis Bottom on horizontal layout places child at bottom", "[arrange][crossaxis]" )
{
    f32 pos = AlignCrossAxis( 20.0f, 0.0f, 100.0f, EAlign::BottomLeft, /*isHz=*/true );
    REQUIRE( pos == Catch::Approx( 80.0f ) ); // 100-20
}

TEST_CASE( "AlignCrossAxis Left on vertical layout places child at left edge", "[arrange][crossaxis]" )
{
    // Vertical layout → cross axis is horizontal → checking Left flag
    f32 pos = AlignCrossAxis( 30.0f, 0.0f, 200.0f, EAlign::TopLeft, /*isHz=*/false );
    REQUIRE( pos == Catch::Approx( 0.0f ) );
}

TEST_CASE( "AlignCrossAxis HCenter on vertical layout centers child horizontally", "[arrange][crossaxis]" )
{
    f32 pos = AlignCrossAxis( 30.0f, 0.0f, 200.0f, EAlign::Center, /*isHz=*/false );
    REQUIRE( pos == Catch::Approx( 85.0f ) ); // (200-30)/2
}

TEST_CASE( "AlignCrossAxis Right on vertical layout places child at right edge", "[arrange][crossaxis]" )
{
    f32 pos = AlignCrossAxis( 30.0f, 0.0f, 200.0f, EAlign::CenterRight, /*isHz=*/false );
    REQUIRE( pos == Catch::Approx( 170.0f ) ); // 200-30
}

TEST_CASE( "AlignCrossAxis with non-zero parent position offsets result", "[arrange][crossaxis]" )
{
    f32 pos = AlignCrossAxis( 20.0f, 50.0f, 100.0f, EAlign::Center, /*isHz=*/true );
    REQUIRE( pos == Catch::Approx( 90.0f ) ); // 50 + (100-20)/2 = 50+40 = 90
}

// =============================================================================
// ResolveAlign
// =============================================================================

TEST_CASE( "ResolveAlign returns parent ChildAlign when child SelfAlign is Inherit", "[arrange][resolve]" )
{
    LayoutNode parent{};
    parent.Style.ChildAlign = EAlign::Center;

    LayoutNode child{};
    child.Style.SelfAlign = EAlign::Inherit;

    EAlign result = ResolveAlign( child, parent );
    REQUIRE( result == EAlign::Center );
}

TEST_CASE( "ResolveAlign returns child SelfAlign when it is not Inherit", "[arrange][resolve]" )
{
    LayoutNode parent{};
    parent.Style.ChildAlign = EAlign::Center;

    LayoutNode child{};
    child.Style.SelfAlign = EAlign::BottomRight;

    EAlign result = ResolveAlign( child, parent );
    REQUIRE( result == EAlign::BottomRight );
}

// =============================================================================
// ArrangeLayoutNode – FinalRect is set to allocated rect
// =============================================================================

TEST_CASE( "ArrangeLayoutNode stores the allocated rect as FinalRect", "[arrange][LayoutNode]" )
{
    LayoutNode w{};
    Rectf rect{ .Origin = { 10.0f, 20.0f }, .Size = { 150.0f, 80.0f } };
    ArrangeLayoutNode( w, rect );
    RequireRect( w.Layout.FinalRect, { 10.0f, 20.0f }, { 150.0f, 80.0f } );
}

// =============================================================================
// ArrangeOverlay – children aligned within parent rect
// =============================================================================

TEST_CASE( "ArrangeOverlay TopLeft child is placed at container origin", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 50.0f };
    child.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    RequireRect( child.Layout.FinalRect, { 0.0f, 0.0f }, { 50.0f, 30.0f } );
}

TEST_CASE( "ArrangeOverlay Center child is placed at the container center", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::Center;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 40.0f };
    child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // offset x = (200-40)/2 = 80, y = (100-20)/2 = 40
    RequireRect( child.Layout.FinalRect, { 80.0f, 40.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeOverlay BottomRight child is placed at container bottom-right", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::BottomRight;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 30.0f };
    child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    RequireRect( child.Layout.FinalRect, { 170.0f, 80.0f }, { 30.0f, 20.0f } );
}

TEST_CASE( "ArrangeOverlay child SelfAlign overrides parent ChildAlign", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 40.0f };
    child.Style.FixedHeight = Unit{ 20.0f };
    child.Style.SelfAlign   = EAlign::Center;

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    RequireRect( child.Layout.FinalRect, { 80.0f, 40.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeOverlay multiple children each aligned independently", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode c1{};
    c1.Style.WidthMode   = ESizing::Fixed; c1.Style.HeightMode  = ESizing::Fixed;
    c1.Style.FixedWidth  = Unit{ 20.0f };             c1.Style.FixedHeight  = Unit{ 10.0f };
    c1.Style.SelfAlign   = EAlign::TopLeft;

    LayoutNode c2{};
    c2.Style.WidthMode   = ESizing::Fixed; c2.Style.HeightMode  = ESizing::Fixed;
    c2.Style.FixedWidth  = Unit{ 20.0f };             c2.Style.FixedHeight  = Unit{ 10.0f };
    c2.Style.SelfAlign   = EAlign::BottomRight;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 200.0f, 100.0f } );

    RequireRect( c1.Layout.FinalRect, { 0.0f,  0.0f  }, { 20.0f, 10.0f } );
    RequireRect( c2.Layout.FinalRect, { 180.0f, 90.0f }, { 20.0f, 10.0f } );
}

TEST_CASE( "ArrangeOverlay with padding reduces inner area for children", "[arrange][overlay]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::TopLeft;
    parent.Style.Padding    = Edges::All( Unit{ 10.0f } );

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 30.0f };
    child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 100.0f, 80.0f } );

    // inner rect = 80x60 starting at (10,10)
    RequireRect( child.Layout.FinalRect, { 10.0f, 10.0f }, { 30.0f, 20.0f } );
}

// =============================================================================
// ArrangeLinear – Horizontal layout
// =============================================================================

TEST_CASE( "ArrangeLinear Horizontal places children side by side from left", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 80.0f }; c2.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 400.0f, 100.0f } );

    // c1 at (0,0), c2 at (50,0)
    RequireRect( c1.Layout.FinalRect, { 0.0f,  0.0f }, { 50.0f, 30.0f } );
    RequireRect( c2.Layout.FinalRect, { 50.0f, 0.0f }, { 80.0f, 30.0f } );
}

TEST_CASE( "ArrangeLinear Horizontal with spacing adds gap between children", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.Spacing    = Unit{ 15.0f };
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 40.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 400.0f, 100.0f } );

    // c1 at 0, c2 at 0 + 15(spacing) + 40 = 55
    RequireRect( c1.Layout.FinalRect, { 0.0f,  0.0f }, { 40.0f, 20.0f } );
    RequireRect( c2.Layout.FinalRect, { 55.0f, 0.0f }, { 60.0f, 20.0f } );
}

TEST_CASE( "ArrangeLinear Horizontal cross-axis VCenter aligns child vertically", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::Center;   // VCenter + HCenter

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 50.0f };
    child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // Cross-axis (vertical): (100 - 20)/2 = 40
    REQUIRE( child.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 40.0f ) );
}

TEST_CASE( "ArrangeLinear Horizontal child with VStretch fills cross axis", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 50.0f };
    child.Style.FixedHeight = Unit{ 20.0f };
    child.Style.SelfAlign   = EAlign::VStretch;

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    REQUIRE( child.Layout.FinalRect.Size[1] .ToFloat() == Catch::Approx( 100.0f ) );
}

TEST_CASE( "ArrangeLinear Horizontal with padding offsets children inside inner rect", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;
    parent.Style.Padding    = Edges::All( Unit{ 10.0f } );

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 50.0f };
    child.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // inner rect starts at (10,10)
    RequireRect( child.Layout.FinalRect, { 10.0f, 10.0f }, { 50.0f, 30.0f } );
}

TEST_CASE( "ArrangeLinear Horizontal single-child margin shifts position and shrinks size", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 60.0f };
    child.Style.FixedHeight = Unit{ 40.0f };
    child.Style.Margin      = Edges::Asymmetric( Unit{ 5.0f }, Unit{ 10.0f }, Unit{ 5.0f }, Unit{ 8.0f } ); // top=5, right=10, bottom=5, left=8

    parent.PushBackChild( child );

    DoLayout( parent, { 300.0f, 100.0f } );

    // origin shifted by margin: x += left=8, y += top=5; size shrunk by margin: w -= 8+10=18, h -= 5+5=10
    RequireRect( child.Layout.FinalRect, { 8.0f, 5.0f }, { 42.0f, 30.0f } );
}

TEST_CASE( "ArrangeLinear Horizontal multiple children cursor advances correctly", "[arrange][linear][horizontal]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;
    parent.Style.Spacing    = Unit{ 5.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 20.0f }; c1.Style.FixedHeight = Unit{ 10.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 30.0f }; c2.Style.FixedHeight = Unit{ 10.0f };

    LayoutNode c3{}; c3.Style.WidthMode = ESizing::Fixed; c3.Style.HeightMode = ESizing::Fixed;
    c3.Style.FixedWidth = Unit{ 40.0f }; c3.Style.FixedHeight = Unit{ 10.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );
    parent.PushBackChild( c3 );

    DoLayout( parent, { 400.0f, 100.0f } );

    // c1 at 0, c2 at 0+5+20=25, c3 at 25+5+30=60
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( c2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 25.0f ) );
    REQUIRE( c3.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 60.0f ) );
}

// =============================================================================
// ArrangeLinear – Vertical layout
// =============================================================================

TEST_CASE( "ArrangeLinear Vertical places children top to bottom", "[arrange][linear][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 50.0f }; c2.Style.FixedHeight = Unit{ 40.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 200.0f, 400.0f } );

    RequireRect( c1.Layout.FinalRect, { 0.0f, 0.0f  }, { 50.0f, 30.0f } );
    RequireRect( c2.Layout.FinalRect, { 0.0f, 30.0f }, { 50.0f, 40.0f } );
}

TEST_CASE( "ArrangeLinear Vertical with spacing adds gap between children", "[arrange][linear][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.Spacing    = Unit{ 10.0f };
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 20.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 50.0f }; c2.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 200.0f, 400.0f } );

    // c1 at y=0, c2 at y = 0+10(spacing)+20 = 30
    RequireRect( c1.Layout.FinalRect, { 0.0f, 0.0f  }, { 50.0f, 20.0f } );
    RequireRect( c2.Layout.FinalRect, { 0.0f, 30.0f }, { 50.0f, 30.0f } );
}

TEST_CASE( "ArrangeLinear Vertical cross-axis HCenter centers child horizontally", "[arrange][linear][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.ChildAlign = EAlign::Center;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 40.0f };
    child.Style.FixedHeight = Unit{ 20.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // cross axis is horizontal: (200 - 40)/2 = 80
    REQUIRE( child.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 80.0f ) );
}

TEST_CASE( "ArrangeLinear Vertical child with HStretch fills cross axis", "[arrange][linear][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 40.0f };
    child.Style.FixedHeight = Unit{ 20.0f };
    child.Style.SelfAlign   = EAlign::HStretch;

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    REQUIRE( child.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 200.0f ) );
}

// =============================================================================
// FlexGrow
// =============================================================================

TEST_CASE( "FlexGrow single child takes all leftover space", "[arrange][flex]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 100.0f }; c1.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 50.0f }; c2.Style.FixedHeight = Unit{ 50.0f };
    c2.Style.FlexGrow = 1.0f; // takes leftover

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 300.0f, 50.0f } );

    // available = 300, fixed = 100+50 = 150, leftover = 150 → c2 gets 50+150 = 200
    REQUIRE( c2.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 200.0f ) );
}

TEST_CASE( "FlexGrow two equal-weight children split leftover equally", "[arrange][flex]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 0.0f }; c1.Style.FixedHeight = Unit{ 50.0f };
    c1.Style.FlexGrow = 1.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 0.0f }; c2.Style.FixedHeight = Unit{ 50.0f };
    c2.Style.FlexGrow = 1.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 300.0f, 50.0f } );

    // both start at 0, leftover = 300, each gets 150
    REQUIRE( c1.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 150.0f ) );
    REQUIRE( c2.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 150.0f ) );
}

TEST_CASE( "FlexGrow 2:1 ratio distributes leftover proportionally", "[arrange][flex]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 0.0f }; c1.Style.FixedHeight = Unit{ 50.0f };
    c1.Style.FlexGrow = 2.0f;

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 0.0f }; c2.Style.FixedHeight = Unit{ 50.0f };
    c2.Style.FlexGrow = 1.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 300.0f, 50.0f } );

    // leftover = 300, c1 gets 2/3 * 300 = 200, c2 gets 1/3 * 300 = 100
    REQUIRE( c1.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 200.0f ) );
    REQUIRE( c2.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 100.0f ) );
}

TEST_CASE( "FlexGrow is clamped by SizeConstraints max", "[arrange][flex]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 0.0f };
    child.Style.FixedHeight = Unit{ 50.0f };
    child.Style.FlexGrow    = 1.0f;
    child.Style.SizeConstraints = Constraints::AtMost( ToUnitVec2( Vec2f{ 100.0f, 100.0f } ) );

    parent.PushBackChild( child );

    DoLayout( parent, { 300.0f, 50.0f } );

    // Would get 300, but clamped to max 100
    REQUIRE( child.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 100.0f ) );
}

TEST_CASE( "FlexGrow Vertical: child grows along vertical axis", "[arrange][flex][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 100.0f };
    parent.Style.FixedHeight = Unit{ 200.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 100.0f }; c1.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 100.0f }; c2.Style.FixedHeight = Unit{ 0.0f };
    c2.Style.FlexGrow = 1.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 100.0f, 200.0f } );

    // available = 200, fixed = 50, leftover = 150 → c2 gets 150
    REQUIRE( c2.Layout.FinalRect.Size[1] .ToFloat() == Catch::Approx( 150.0f ) );
}

// =============================================================================
// ArrangeAnchored
// =============================================================================

TEST_CASE( "ArrangeAnchored TopLeft point anchor places child at top-left of parent", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode      = ESizing::Fixed;
    child.Style.HeightMode     = ESizing::Fixed;
    child.Style.FixedWidth     = Unit{ 40.0f };
    child.Style.FixedHeight    = Unit{ 20.0f };
    child.Style.PositionMode   = EPositioning::Anchored;
    child.Style.PositionAnchor         = Anchor::TopLeft();  // Min=(0,0), Pivot=(0,0)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    RequireRect( child.Layout.FinalRect, { 0.0f, 0.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeAnchored Center point anchor places child at parent center", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 40.0f };
    child.Style.FixedHeight  = Unit{ 20.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::Center(); // Min=(0.5,0.5), Pivot=(0.5,0.5)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // anchorPoint = (100, 50); origin = (100,50) - (40*0.5, 20*0.5) = (80, 40)
    RequireRect( child.Layout.FinalRect, { 80.0f, 40.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeAnchored BottomRight point anchor places child at bottom-right", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 40.0f };
    child.Style.FixedHeight  = Unit{ 20.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::BottomRight(); // Min=(1,1), Pivot=(1,1)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // anchorPoint = (200,100); origin = (200,100) - (40*1, 20*1) = (160, 80)
    RequireRect( child.Layout.FinalRect, { 160.0f, 80.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeAnchored StretchAll fills the entire parent", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::StretchAll(); // Min=(0,0), Max=(1,1), offset=(0,0)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // finalMin = (0,0) + (200,100)*(0,0) + (0,0) = (0,0)
    // finalMax = (0,0) + (200,100)*(1,1) - (0,0) = (200,100)
    RequireRect( child.Layout.FinalRect, { 0.0f, 0.0f }, { 200.0f, 100.0f } );
}

TEST_CASE( "ArrangeAnchored with pixel offset shifts position", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 40.0f };
    child.Style.FixedHeight  = Unit{ 20.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    // Point anchor at top-left with a pixel offset of (10, 5)
    child.Style.PositionAnchor = MakeAnchor( { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 10.0f, 5.0f } );

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // anchorPoint = (0,0) + offset(10,5) = (10,5); origin = (10,5) - (0,0) = (10,5)
    RequireRect( child.Layout.FinalRect, { 10.0f, 5.0f }, { 40.0f, 20.0f } );
}

TEST_CASE( "ArrangeAnchored StretchTop stretches across top edge with zero height", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::StretchTop(); // Min=(0,0), Max=(1,0)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // stretch: finalMin=(0,0), finalMax=(200,0) → size=(200,0)
    REQUIRE( child.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 200.0f ) );
    REQUIRE( child.Layout.FinalRect.Size[1] .ToFloat() == Catch::Approx( 0.0f ) );
    REQUIRE( child.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 0.0f ) );
}

TEST_CASE( "ArrangeAnchored anchored child does not affect parent content size", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Content;
    parent.Style.HeightMode = ESizing::Content;

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 300.0f };
    child.Style.FixedHeight  = Unit{ 200.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::TopLeft();

    parent.PushBackChild( child );

    Vec2f desired = MeasureLayoutNode( parent, { 400.0f, 300.0f } );
    // Anchored child is excluded from content measurement → parent stays 0x0
    RequireApproxEqual( desired, Vec2f( 0.0f, 0.0f ) );
}

// =============================================================================
// Visibility – Collapsed during arrangement
// =============================================================================

TEST_CASE( "ArrangeLinear Collapsed child is skipped and does not advance cursor", "[arrange][visibility]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 50.0f }; c2.Style.FixedHeight = Unit{ 30.0f };
    c2.Style.Visibility = EVisibility::Collapsed;

    LayoutNode c3{}; c3.Style.WidthMode = ESizing::Fixed; c3.Style.HeightMode = ESizing::Fixed;
    c3.Style.FixedWidth = Unit{ 70.0f }; c3.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );
    parent.PushBackChild( c3 );

    // Measure first so DesiredSizes are set
    MeasureLayoutNode( parent, { 400.0f, 100.0f } );
    RatUI::ArrangeLayoutNode( parent, ToUnitRect( Rectf{ .Origin = { 0.0f, 0.0f }, .Size = { 400.0f, 100.0f } } ) );

    // c2 is collapsed, so c3 should start right after c1 (no gap for c2)
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( c3.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 50.0f ) );
}

TEST_CASE( "ArrangeLinear Hidden child still occupies space in layout", "[arrange][visibility]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 30.0f };
    c2.Style.Visibility = EVisibility::Hidden; // affects layout, not rendered

    LayoutNode c3{}; c3.Style.WidthMode = ESizing::Fixed; c3.Style.HeightMode = ESizing::Fixed;
    c3.Style.FixedWidth = Unit{ 40.0f }; c3.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );
    parent.PushBackChild( c3 );

    MeasureLayoutNode( parent, { 400.0f, 100.0f } );
    RatUI::ArrangeLayoutNode( parent, ToUnitRect( Rectf{ .Origin = { 0.0f, 0.0f }, .Size = { 400.0f, 100.0f } } ) );

    // c1@0, c2@50, c3@110
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f   ) );
    REQUIRE( c2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 50.0f  ) );
    REQUIRE( c3.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 110.0f ) );
}

// =============================================================================
// Nested hierarchy arrange
// =============================================================================

//TEST_CASE( "ArrangeLayoutNode nested Horizontal inside Vertical positions grandchildren correctly", "[arrange][nested]" )
//{
//    // Root: Vertical
//    LayoutNode root{};
//    root.Style.LayoutType = ELayoutType::Vertical;
//    root.Style.ChildAlign = EAlign::TopLeft;
//    root.Style.WidthMode  = ESizing::Fixed;
//    root.Style.HeightMode = ESizing::Fixed;
//    root.Style.FixedWidth  = Unit{ 200.0f };
//    root.Style.FixedHeight = Unit{ 200.0f };
//
//    // row: Horizontal with two fixed children
//    LayoutNode row{};
//    row.Style.LayoutType = ELayoutType::Horizontal;
//    row.Style.ChildAlign = EAlign::TopLeft;
//    row.Style.WidthMode  = ESizing::Fill; row.Style.PercentWidth  = 1.0f;
//    row.Style.HeightMode = ESizing::Fixed; row.Style.FixedHeight  = Unit{ 50.0f };
//
//    LayoutNode gc1{}; gc1.Style.WidthMode = ESizing::Fixed; gc1.Style.HeightMode = ESizing::Fixed;
//    gc1.Style.FixedWidth = Unit{ 60.0f }; gc1.Style.FixedHeight = Unit{ 50.0f };
//
//    LayoutNode gc2{}; gc2.Style.WidthMode = ESizing::Fixed; gc2.Style.HeightMode = ESizing::Fixed;
//    gc2.Style.FixedWidth = Unit{ 80.0f }; gc2.Style.FixedHeight = Unit{ 50.0f };
//
//    row.PushBackChild( gc1 );
//    row.PushBackChild( gc2 );
//
//    // single fixed child below the row
//    LayoutNode below{};
//    below.Style.WidthMode   = ESizing::Fixed;
//    below.Style.HeightMode  = ESizing::Fixed;
//    below.Style.FixedWidth  = Unit{ 100.0f };
//    below.Style.FixedHeight = Unit{ 30.0f };
//
//    root.PushBackChild( row );
//    root.PushBackChild( below );
//
//    DoLayout( root, { 200.0f, 200.0f } );
//
//    // row is at y=0, height=50 → below is at y=50
//    REQUIRE( row.Layout.FinalRect.Origin[1]   .ToFloat() == Catch::Approx( 0.0f  ) );
//    REQUIRE( below.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 50.0f ) );
//
//    // grandchildren are inside row (y=0 to y=50)
//    REQUIRE( gc1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f  ) );
//    REQUIRE( gc1.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 0.0f  ) );
//    REQUIRE( gc2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 60.0f ) );
//    REQUIRE( gc2.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 0.0f  ) );
//}

TEST_CASE( "ArrangeLayoutNode deeply nested: 3 levels of Overlay LayoutNodes set FinalRect", "[arrange][nested]" )
{
    LayoutNode l1{};
    l1.Style.LayoutType = ELayoutType::Overlay;
    l1.Style.WidthMode  = ESizing::Fixed; l1.Style.HeightMode = ESizing::Fixed;
    l1.Style.FixedWidth = Unit{ 200.0f }; l1.Style.FixedHeight = Unit{ 200.0f };

    LayoutNode l2{};
    l2.Style.LayoutType = ELayoutType::Overlay;
    l2.Style.WidthMode  = ESizing::Fixed; l2.Style.HeightMode = ESizing::Fixed;
    l2.Style.FixedWidth = Unit{ 100.0f }; l2.Style.FixedHeight = Unit{ 100.0f };
    l2.Style.SelfAlign  = EAlign::Center;

    LayoutNode l3{};
    l3.Style.WidthMode  = ESizing::Fixed; l3.Style.HeightMode = ESizing::Fixed;
    l3.Style.FixedWidth = Unit{ 40.0f }; l3.Style.FixedHeight = Unit{ 40.0f };
    l3.Style.SelfAlign  = EAlign::Center;

    l2.PushBackChild( l3 );
    l1.PushBackChild( l2 );

    DoLayout( l1, { 200.0f, 200.0f } );

    // l2 centered in l1 200x200 → origin (50, 50)
    RequireRect( l2.Layout.FinalRect, { 50.0f, 50.0f }, { 100.0f, 100.0f } );

    // l3 centered in l2 100x100 at (50,50) → origin (50+(100-40)/2, 50+(100-40)/2) = (80, 80)
    RequireRect( l3.Layout.FinalRect, { 80.0f, 80.0f }, { 40.0f, 40.0f } );
}

// =============================================================================
// Measure + Arrange round-trip: Collapsed LayoutNodes
// =============================================================================

TEST_CASE( "Collapsed LayoutNode has zero desired size and empty FinalRect after full layout", "[arrange][visibility][roundtrip]" )
{
    LayoutNode w{};
    w.Style.WidthMode   = ESizing::Fixed;
    w.Style.HeightMode  = ESizing::Fixed;
    w.Style.FixedWidth  = Unit{ 100.0f };
    w.Style.FixedHeight = Unit{ 50.0f };
    w.Style.Visibility = EVisibility::Collapsed;

    Vec2f desired = MeasureLayoutNode( w, { 500.0f, 500.0f } );
    RequireApproxEqual( desired, Vec2f( 0.0f, 0.0f ) );
}

// =============================================================================
// Measure + Arrange round-trip: padding interacts with flex
// =============================================================================

TEST_CASE( "FlexGrow respects padding: leftover excludes padding from available space", "[arrange][flex][padding]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };
    parent.Style.Padding     = Edges::Axis( Unit{ 25.0f }, Unit{ 0.0f } ); // 25px each side = 50px total H padding

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 0.0f };
    child.Style.FixedHeight = Unit{ 50.0f };
    child.Style.FlexGrow    = 1.0f;

    parent.PushBackChild( child );

    DoLayout( parent, { 300.0f, 50.0f } );

    // inner width = 300 - 50 = 250; child starts at (25, 0) and gets 250 width
    REQUIRE( child.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 25.0f  ) );
    REQUIRE( child.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 250.0f ) );
}

// =============================================================================
// Measure + Arrange round-trip: margin with spacing
// =============================================================================

TEST_CASE( "ArrangeLinear Horizontal margin and spacing interact correctly", "[arrange][linear][margin][spacing]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.Spacing    = Unit{ 10.0f };
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };
    c1.Style.Margin = Edges::Asymmetric( Unit{ 0.0f }, Unit{ 5.0f }, Unit{ 0.0f }, Unit{ 5.0f } ); // top=0, right=5, bottom=0, left=5

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 60.0f }; c2.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 400.0f, 100.0f } );

    // c1: cursor starts at 0; origin x = 0 + margin.left(5) = 5; size.x = 50 - margin.H(10) = 40
    // After c1: cursor += spacing(10) + (c1.DesiredSize[0]=50 + c1.margin.H=10) = 0 + 10 + 60 = 70
    // c2: origin x = 70 + margin.left(0) = 70; size.x = 60
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 5.0f  ) );
    REQUIRE( c1.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 40.0f ) );
    REQUIRE( c2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 70.0f ) );
    REQUIRE( c2.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 60.0f ) );
}

// =============================================================================
// Content-sized parent with mixed Fixed children and spacing
// =============================================================================

TEST_CASE( "Content-sized parent resizes correctly after full Measure+Arrange round-trip", "[arrange][roundtrip]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Content;
    parent.Style.HeightMode = ESizing::Content;
    parent.Style.Spacing    = Unit{ 5.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 100.0f }; c1.Style.FixedHeight = Unit{ 40.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 80.0f }; c2.Style.FixedHeight = Unit{ 60.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 1000.0f, 1000.0f } );

    // Parent desired: w=100+5+80=185, h=max(40,60)=60
    RequireApproxEqual( parent.Layout.DesiredSize, Vec2f( 185.0f, 60.0f ) );

    // c1 at (0,0), c2 at (100+5, 0) = (105, 0) — but they are laid out inside a rect of 1000x1000
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f   ) );
    REQUIRE( c2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 105.0f ) );
}

// =============================================================================
// ArrangeAnchored – remaining preset anchors and edge cases
// =============================================================================

TEST_CASE( "ArrangeAnchored StretchBottom stretches across bottom edge with zero height", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::StretchBottom(); // Min=(0,1), Max=(1,1)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // Stretch bottom edge: finalMin=(0,100), finalMax=(200,100) → size=(200,0)
    REQUIRE( child.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 200.0f ) );
    REQUIRE( child.Layout.FinalRect.Size[1]   .ToFloat() == Catch::Approx( 0.0f   ) );
    REQUIRE( child.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 100.0f ) );
}

TEST_CASE( "ArrangeAnchored StretchLeft stretches along left edge", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::StretchLeft(); // Min=(0,0), Max=(0,1)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // Stretch left edge: x is a point anchor (stretchX=false), y stretches full height
    REQUIRE( child.Layout.FinalRect.Size[1]   .ToFloat() == Catch::Approx( 100.0f ) );
    REQUIRE( child.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 0.0f   ) );
    REQUIRE( child.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f   ) );
}

TEST_CASE( "ArrangeAnchored StretchRight stretches along right edge", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::StretchRight(); // Min=(1,0), Max=(1,1)

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // Stretch right edge: x is a point anchor at 1.0, y stretches full height
    REQUIRE( child.Layout.FinalRect.Size[1]   .ToFloat() == Catch::Approx( 100.0f ) );
    REQUIRE( child.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 0.0f   ) );
    REQUIRE( child.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 200.0f ) );
}

TEST_CASE( "ArrangeAnchored StretchAll with inward offset shrinks the stretch rect symmetrically", "[arrange][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 0.0f };
    child.Style.FixedHeight  = Unit{ 0.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    // StretchAll with offset=(10,5) pushes each edge inward by the offset value
    child.Style.PositionAnchor = MakeAnchor( { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.5f, 0.5f }, { 10.0f, 5.0f } );

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    // X: origin = 0 + 200*0 + 10 = 10; right = 0 + 200*1 - 10 = 190; w = 180
    // Y: origin = 0 + 100*0 +  5 =  5; bottom = 0 + 100*1 -  5 =  95; h =  90
    RequireRect( child.Layout.FinalRect, { 10.0f, 5.0f }, { 180.0f, 90.0f } );
}

TEST_CASE( "ArrangeAnchored with non-zero parent container origin positions child correctly", "[arrange][anchored]" )
{
    // Parent rect starts at (50, 30), not at (0,0)
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode    = ESizing::Fixed;
    child.Style.HeightMode   = ESizing::Fixed;
    child.Style.FixedWidth   = Unit{ 40.0f };
    child.Style.FixedHeight  = Unit{ 20.0f };
    child.Style.PositionMode = EPositioning::Anchored;
    child.Style.PositionAnchor       = Anchor::Center(); // Min=Max=(0.5,0.5), Pivot=(0.5,0.5)

    parent.PushBackChild( child );

    // Arrange the parent rect starting at (50,30)
    MeasureLayoutNode( parent, { 200.0f, 100.0f } );
    ArrangeLayoutNode( parent, { .Origin = { 50.0f, 30.0f }, .Size = { 200.0f, 100.0f } } );

    // anchorX = 50 + 200*0.5 = 150; originX = 150 - 40*0.5 = 130
    // anchorY = 30 + 100*0.5 =  80; originY =  80 - 20*0.5 =  70
    RequireRect( child.Layout.FinalRect, { 130.0f, 70.0f }, { 40.0f, 20.0f } );
}

// =============================================================================
// ArrangeLinear – mixed anchored + flow children
// =============================================================================

TEST_CASE( "ArrangeLinear Horizontal anchored children do not advance the flow cursor", "[arrange][linear][anchored]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.ChildAlign = EAlign::TopLeft;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 400.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 30.0f };

    // Anchored child – should NOT advance the horizontal cursor
    LayoutNode anchored{};
    anchored.Style.WidthMode    = ESizing::Fixed;
    anchored.Style.HeightMode   = ESizing::Fixed;
    anchored.Style.FixedWidth   = Unit{ 200.0f };
    anchored.Style.FixedHeight  = Unit{ 200.0f };
    anchored.Style.PositionMode = EPositioning::Anchored;
    anchored.Style.PositionAnchor       = Anchor::TopLeft();

    LayoutNode c3{}; c3.Style.WidthMode = ESizing::Fixed; c3.Style.HeightMode = ESizing::Fixed;
    c3.Style.FixedWidth = Unit{ 40.0f }; c3.Style.FixedHeight = Unit{ 30.0f };

    parent.PushBackChild( c1 );
    parent.PushBackChild( anchored );
    parent.PushBackChild( c3 );

    DoLayout( parent, { 400.0f, 100.0f } );

    // c1 at x=0, c3 immediately after c1 at x=50 (anchored child has no effect on flow)
    REQUIRE( c1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( c3.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 50.0f ) );
}

// =============================================================================
// ArrangeLinear – FlexGrow accounts for spacing in available space
// =============================================================================

TEST_CASE( "ArrangeLinear FlexGrow with spacing: spacing reduces available flex space", "[arrange][flex][spacing]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 300.0f };
    parent.Style.FixedHeight = Unit{ 50.0f };
    parent.Style.Spacing     = Unit{ 10.0f };

    LayoutNode c1{}; c1.Style.WidthMode = ESizing::Fixed; c1.Style.HeightMode = ESizing::Fixed;
    c1.Style.FixedWidth = Unit{ 50.0f }; c1.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode c2{}; c2.Style.WidthMode = ESizing::Fixed; c2.Style.HeightMode = ESizing::Fixed;
    c2.Style.FixedWidth = Unit{ 0.0f }; c2.Style.FixedHeight = Unit{ 50.0f };
    c2.Style.FlexGrow = 1.0f;

    parent.PushBackChild( c1 );
    parent.PushBackChild( c2 );

    DoLayout( parent, { 300.0f, 50.0f } );

    // available = 300 - spacing*(numFlow-1) = 300 - 10*1 = 290
    // totalFixed = 50 (c1)
    // leftover = 290 - 50 = 240 → c2 gets 240
    REQUIRE( c2.Layout.FinalRect.Size[0]   .ToFloat() == Catch::Approx( 240.0f ) );
    REQUIRE( c2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 60.0f  ) ); // 0 + spacing(10) + c1(50) = 60
}

// =============================================================================
// ArrangeLinear Vertical – margin on child
// =============================================================================

TEST_CASE( "ArrangeLinear Vertical single-child margin shifts position and shrinks size", "[arrange][linear][vertical]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Vertical;
    parent.Style.ChildAlign = EAlign::TopLeft;

    LayoutNode child{};
    child.Style.WidthMode   = ESizing::Fixed;
    child.Style.HeightMode  = ESizing::Fixed;
    child.Style.FixedWidth  = Unit{ 80.0f };
    child.Style.FixedHeight = Unit{ 60.0f };
    child.Style.Margin      = Edges::Asymmetric( Unit{ 8.0f }, Unit{ 0.0f }, Unit{ 12.0f }, Unit{ 0.0f } ); // top=8, bottom=12

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 300.0f } );

    // origin y += margin.top=8; size.h -= margin.vertical=20 → h=40
    RequireRect( child.Layout.FinalRect, { 0.0f, 8.0f }, { 80.0f, 40.0f } );
}

// =============================================================================
// ArrangeOverlay – collapsed child
// =============================================================================

TEST_CASE( "ArrangeOverlay Collapsed child gets zero-size FinalRect and non-collapsed sibling is placed correctly", "[arrange][overlay][visibility]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode collapsed{};
    collapsed.Style.WidthMode   = ESizing::Fixed;
    collapsed.Style.HeightMode  = ESizing::Fixed;
    collapsed.Style.FixedWidth  = Unit{ 100.0f };
    collapsed.Style.FixedHeight = Unit{ 50.0f };
    collapsed.Style.Visibility = EVisibility::Collapsed;

    LayoutNode visible{};
    visible.Style.WidthMode   = ESizing::Fixed;
    visible.Style.HeightMode  = ESizing::Fixed;
    visible.Style.FixedWidth  = Unit{ 40.0f };
    visible.Style.FixedHeight = Unit{ 20.0f };
    visible.Style.SelfAlign   = EAlign::BottomRight;

    parent.PushBackChild( collapsed );
    parent.PushBackChild( visible );

    DoLayout( parent, { 200.0f, 100.0f } );

    // Collapsed child has DesiredSize=0×0 and gets a 0×0 FinalRect
    REQUIRE( collapsed.Layout.DesiredSize[0] .ToFloat() == Catch::Approx( 0.0f ) );
    REQUIRE( collapsed.Layout.DesiredSize[1] .ToFloat() == Catch::Approx( 0.0f ) );
    REQUIRE( collapsed.Layout.FinalRect.Size[0] .ToFloat() == Catch::Approx( 0.0f ) );
    REQUIRE( collapsed.Layout.FinalRect.Size[1] .ToFloat() == Catch::Approx( 0.0f ) );

    // Visible child at BottomRight: (160, 80)
    RequireRect( visible.Layout.FinalRect, { 160.0f, 80.0f }, { 40.0f, 20.0f } );
}

// =============================================================================
// Nested Horizontal inside Vertical – full round-trip
// =============================================================================

TEST_CASE( "ArrangeLayoutNode nested Horizontal inside Vertical positions grandchildren correctly", "[arrange][nested]" )
{
    // root: Vertical, Fixed 200×200
    LayoutNode root{};
    root.Style.LayoutType = ELayoutType::Vertical;
    root.Style.ChildAlign = EAlign::TopLeft;
    root.Style.WidthMode  = ESizing::Fixed;
    root.Style.HeightMode = ESizing::Fixed;
    root.Style.FixedWidth  = Unit{ 200.0f };
    root.Style.FixedHeight = Unit{ 200.0f };

    // row: Horizontal, Fixed width=200, Fixed height=50
    LayoutNode row{};
    row.Style.LayoutType = ELayoutType::Horizontal;
    row.Style.ChildAlign = EAlign::TopLeft;
    row.Style.WidthMode  = ESizing::Fixed;
    row.Style.HeightMode = ESizing::Fixed;
    row.Style.FixedWidth  = Unit{ 200.0f };
    row.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode gc1{}; gc1.Style.WidthMode = ESizing::Fixed; gc1.Style.HeightMode = ESizing::Fixed;
    gc1.Style.FixedWidth = Unit{ 60.0f }; gc1.Style.FixedHeight = Unit{ 50.0f };

    LayoutNode gc2{}; gc2.Style.WidthMode = ESizing::Fixed; gc2.Style.HeightMode = ESizing::Fixed;
    gc2.Style.FixedWidth = Unit{ 80.0f }; gc2.Style.FixedHeight = Unit{ 50.0f };

    row.PushBackChild( gc1 );
    row.PushBackChild( gc2 );

    // Node below the row
    LayoutNode below{};
    below.Style.WidthMode   = ESizing::Fixed;
    below.Style.HeightMode  = ESizing::Fixed;
    below.Style.FixedWidth  = Unit{ 100.0f };
    below.Style.FixedHeight = Unit{ 30.0f };

    root.PushBackChild( row );
    root.PushBackChild( below );

    DoLayout( root, { 200.0f, 200.0f } );

    // row at y=0 height=50, below at y=50
    REQUIRE( row.Layout.FinalRect.Origin[1]   .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( below.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 50.0f ) );

    // gc1 and gc2 inside row (world y=0)
    REQUIRE( gc1.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( gc1.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 0.0f  ) );
    REQUIRE( gc2.Layout.FinalRect.Origin[0] .ToFloat() == Catch::Approx( 60.0f ) );
    REQUIRE( gc2.Layout.FinalRect.Origin[1] .ToFloat() == Catch::Approx( 0.0f  ) );
}

// =============================================================================
// ESizing::Flex – height fills parent in horizontal layout (fallback)
// =============================================================================

TEST_CASE( "ArrangeLinear Horizontal child with Flex HeightMode fills parent height automatically", "[arrange][linear][flex]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Horizontal;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 200.0f };
    parent.Style.FixedHeight = Unit{ 100.0f };

    LayoutNode child{};
    child.Style.WidthMode  = ESizing::Fixed;
    child.Style.HeightMode = ESizing::Flex; // fills parent height in horizontal layout
    child.Style.FixedWidth = Unit{ 50.0f };

    parent.PushBackChild( child );

    DoLayout( parent, { 200.0f, 100.0f } );

    REQUIRE( child.Layout.FinalRect.Size[1] .ToFloat() == Catch::Approx( 100.0f ) );
}

// =============================================================================
// Percent sizing – measure and arrange round-trip
// =============================================================================

TEST_CASE( "ArrangeOverlay child with Percent sizing is positioned using fraction of available size", "[arrange][overlay][percent]" )
{
    LayoutNode parent{};
    parent.Style.LayoutType = ELayoutType::Overlay;
    parent.Style.ChildAlign = EAlign::TopLeft;
    parent.Style.WidthMode  = ESizing::Fixed;
    parent.Style.HeightMode = ESizing::Fixed;
    parent.Style.FixedWidth  = Unit{ 400.0f };
    parent.Style.FixedHeight = Unit{ 200.0f };

    LayoutNode child{};
    child.Style.WidthMode     = ESizing::Percent;
    child.Style.HeightMode    = ESizing::Percent;
    child.Style.PercentWidth  = 0.5f;
    child.Style.PercentHeight = 0.5f;

    parent.PushBackChild( child );

    DoLayout( parent, { 400.0f, 200.0f } );

    // Child measured as 50% of available: 200×100, placed at TopLeft (0,0)
    RequireRect( child.Layout.FinalRect, { 0.0f, 0.0f }, { 200.0f, 100.0f } );
}
