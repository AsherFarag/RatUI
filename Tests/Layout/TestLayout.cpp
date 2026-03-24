/**
 * @file TestLayout.cpp
 * @brief Tests for RatUI layout types: EAlignment, ELayoutType, Visibility, Constraints, Edges, Anchor, LayoutStyle, LayoutResult.
 */

#include "../Common/Common.h"

using namespace RatUI;

// =============================================================================
// EAlignment
// =============================================================================

TEST_CASE( "EAlignment single horizontal flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( Left    & HCenter ) == 0 );
    REQUIRE( ( Left    & Right   ) == 0 );
    REQUIRE( ( HCenter & Right   ) == 0 );
}

TEST_CASE( "EAlignment single vertical flags have distinct bits", "[layout][alignment]" )
{
    REQUIRE( ( Top     & VCenter ) == 0 );
    REQUIRE( ( Top     & Bottom  ) == 0 );
    REQUIRE( ( VCenter & Bottom  ) == 0 );
}

TEST_CASE( "EAlignment combined presets match their component flags", "[layout][alignment]" )
{
    REQUIRE( TopLeft      == ( Top     | Left    ) );
    REQUIRE( TopCenter    == ( Top     | HCenter ) );
    REQUIRE( TopRight     == ( Top     | Right   ) );
    REQUIRE( CenterLeft   == ( VCenter | Left    ) );
    REQUIRE( Center       == ( VCenter | HCenter ) );
    REQUIRE( CenterRight  == ( VCenter | Right   ) );
    REQUIRE( BottomLeft   == ( Bottom  | Left    ) );
    REQUIRE( BottomCenter == ( Bottom  | HCenter ) );
    REQUIRE( BottomRight  == ( Bottom  | Right   ) );
}

// =============================================================================
// Constraints
// =============================================================================

TEST_CASE( "Constraints::Unbounded has zero min and max float max", "[layout][constraints]" )
{
    Constraints c = Constraints::Unbounded();
    REQUIRE( c.MinSize[ 0 ] == 0.0f );
    REQUIRE( c.MinSize[ 1 ] == 0.0f );
    REQUIRE( c.MaxSize[ 0 ] == std::numeric_limits<f32>::max() );
    REQUIRE( c.MaxSize[ 1 ] == std::numeric_limits<f32>::max() );
}

TEST_CASE( "Constraints::Fixed sets min and max to the same size", "[layout][constraints]" )
{
    Constraints c = Constraints::Fixed( Vec2f( 100.0f, 200.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 100.0f, 200.0f ) );
    RequireApproxEqual( c.MaxSize, Vec2f( 100.0f, 200.0f ) );
}

TEST_CASE( "Constraints::AtLeast sets min and leaves max unbounded", "[layout][constraints]" )
{
    Constraints c = Constraints::AtLeast( Vec2f( 50.0f, 75.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 50.0f, 75.0f ) );
    REQUIRE( c.MaxSize[ 0 ] == std::numeric_limits<f32>::max() );
    REQUIRE( c.MaxSize[ 1 ] == std::numeric_limits<f32>::max() );
}

TEST_CASE( "Constraints::AtMost leaves min at zero and sets max", "[layout][constraints]" )
{
    Constraints c = Constraints::AtMost( Vec2f( 300.0f, 400.0f ) );
    RequireApproxEqual( c.MinSize, Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( c.MaxSize, Vec2f( 300.0f, 400.0f ) );
}
// =============================================================================
// Visibility
// =============================================================================

TEST_CASE( "Visibility defaults to Visible", "[layout][visibility]" )
{
    Visibility v{};
    REQUIRE( v.Value == Visibility::Visible );
}

TEST_CASE( "Visibility Visible AffectsLayout and IsRendered", "[layout][visibility]" )
{
    Visibility v{ Visibility::Visible };
    REQUIRE( v.AffectsLayout() );
    REQUIRE( v.IsRendered() );
}

TEST_CASE( "Visibility Hidden AffectsLayout but not IsRendered", "[layout][visibility]" )
{
    Visibility v{ Visibility::Hidden };
    REQUIRE( v.AffectsLayout() );
    REQUIRE_FALSE( v.IsRendered() );
}

TEST_CASE( "Visibility Collapsed does not affect layout and is not rendered", "[layout][visibility]" )
{
    Visibility v{ Visibility::Collapsed };
    REQUIRE_FALSE( v.AffectsLayout() );
    REQUIRE_FALSE( v.IsRendered() );
}

// =============================================================================
// Edges
// =============================================================================

TEST_CASE( "Edges default construction produces all-zero edges", "[layout][edges]" )
{
    Edges e{};
    REQUIRE( e.Top    == 0.0f );
    REQUIRE( e.Right  == 0.0f );
    REQUIRE( e.Bottom == 0.0f );
    REQUIRE( e.Left   == 0.0f );
}

TEST_CASE( "Edges::Uniform sets all edges to the same value", "[layout][edges]" )
{
    Edges e = Edges::Uniform( 10.0f );
    REQUIRE( e.Top    == 10.0f );
    REQUIRE( e.Right  == 10.0f );
    REQUIRE( e.Bottom == 10.0f );
    REQUIRE( e.Left   == 10.0f );
}

TEST_CASE( "Edges::Symmetric sets horizontal and vertical edges", "[layout][edges]" )
{
    Edges e = Edges::Symmetric( 8.0f, 4.0f );
    REQUIRE( e.Left   == 8.0f );
    REQUIRE( e.Right  == 8.0f );
    REQUIRE( e.Top    == 4.0f );
    REQUIRE( e.Bottom == 4.0f );
}

TEST_CASE( "Edges::Asymmetric sets each edge individually", "[layout][edges]" )
{
    Edges e = Edges::Asymmetric( 1.0f, 2.0f, 3.0f, 4.0f );
    REQUIRE( e.Top    == 1.0f );
    REQUIRE( e.Right  == 2.0f );
    REQUIRE( e.Bottom == 3.0f );
    REQUIRE( e.Left   == 4.0f );
}

TEST_CASE( "Edges::Horizontal() returns left plus right", "[layout][edges]" )
{
    Edges e = Edges::Asymmetric( 1.0f, 6.0f, 3.0f, 4.0f );
    REQUIRE( e.Horizontal() == Catch::Approx( 10.0f ) );
}

TEST_CASE( "Edges::Vertical() returns top plus bottom", "[layout][edges]" )
{
    Edges e = Edges::Asymmetric( 2.0f, 6.0f, 8.0f, 4.0f );
    REQUIRE( e.Vertical() == Catch::Approx( 10.0f ) );
}

TEST_CASE( "Edges::Total() returns horizontal and vertical as Vec2f", "[layout][edges]" )
{
    Edges e = Edges::Symmetric( 5.0f, 3.0f );
    RequireApproxEqual( e.Total(), Vec2f( 10.0f, 6.0f ) );
}

TEST_CASE( "Edges::Apply() shrinks rect by insets", "[layout][edges]" )
{
    Rectf r    = Rectf::FromMinMax( Vec2f( 0.0f, 0.0f ), Vec2f( 100.0f, 80.0f ) );
    Edges e    = Edges::Asymmetric( 5.0f, 10.0f, 15.0f, 20.0f );
    Rectf inset = e.Apply( r );
    REQUIRE( inset.Left()   == Catch::Approx( 20.0f ) );
    REQUIRE( inset.Top()    == Catch::Approx( 5.0f  ) );
    REQUIRE( inset.Right()  == Catch::Approx( 90.0f ) );
    REQUIRE( inset.Bottom() == Catch::Approx( 65.0f ) );
}

TEST_CASE( "Edges::Apply() with zero edges leaves rect unchanged", "[layout][edges]" )
{
    Rectf r     = Rectf::FromMinMax( Vec2f( 10.0f, 20.0f ), Vec2f( 110.0f, 120.0f ) );
    Edges e     = Edges::Uniform( 0.0f );
    Rectf result = e.Apply( r );
    RequireApproxEqual( result.Min(), r.Min() );
    RequireApproxEqual( result.Max(), r.Max() );
}

// =============================================================================
// Anchor
// =============================================================================

TEST_CASE( "Anchor default construction produces all-zero fields", "[layout][anchor]" )
{
    Anchor a{};
    RequireApproxEqual( a.Min,    Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Max,    Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Pivot,  Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Offset, Vec2f( 0.0f, 0.0f ) );
}

TEST_CASE( "Anchor::TopLeft has min and max at (0,0)", "[layout][anchor]" )
{
    Anchor a = Anchor::TopLeft();
    RequireApproxEqual( a.Min,   Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Max,   Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Pivot, Vec2f( 0.0f, 0.0f ) );
}

TEST_CASE( "Anchor::Center has min and max at (0.5,0.5)", "[layout][anchor]" )
{
    Anchor a = Anchor::Center();
    RequireApproxEqual( a.Min,   Vec2f( 0.5f, 0.5f ) );
    RequireApproxEqual( a.Max,   Vec2f( 0.5f, 0.5f ) );
    RequireApproxEqual( a.Pivot, Vec2f( 0.5f, 0.5f ) );
}

TEST_CASE( "Anchor::BottomRight has min and max at (1,1)", "[layout][anchor]" )
{
    Anchor a = Anchor::BottomRight();
    RequireApproxEqual( a.Min,   Vec2f( 1.0f, 1.0f ) );
    RequireApproxEqual( a.Max,   Vec2f( 1.0f, 1.0f ) );
    RequireApproxEqual( a.Pivot, Vec2f( 1.0f, 1.0f ) );
}

TEST_CASE( "Anchor::StretchAll spans the full parent", "[layout][anchor]" )
{
    Anchor a = Anchor::StretchAll();
    RequireApproxEqual( a.Min,   Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Max,   Vec2f( 1.0f, 1.0f ) );
    RequireApproxEqual( a.Pivot, Vec2f( 0.5f, 0.5f ) );
}

TEST_CASE( "Anchor::TopCenter has horizontal center anchor", "[layout][anchor]" )
{
    Anchor a = Anchor::TopCenter();
    REQUIRE( a.Min[ 0 ] == Catch::Approx( 0.5f ) );
    REQUIRE( a.Min[ 1 ] == Catch::Approx( 0.0f ) );
    REQUIRE( a.Max[ 0 ] == Catch::Approx( 0.5f ) );
    REQUIRE( a.Max[ 1 ] == Catch::Approx( 0.0f ) );
}

TEST_CASE( "Anchor::StretchTop stretches horizontally along the top edge", "[layout][anchor]" )
{
    Anchor a = Anchor::StretchTop();
    RequireApproxEqual( a.Min, Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Max, Vec2f( 1.0f, 0.0f ) );
}

TEST_CASE( "Anchor::StretchLeft stretches vertically along the left edge", "[layout][anchor]" )
{
    Anchor a = Anchor::StretchLeft();
    RequireApproxEqual( a.Min, Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( a.Max, Vec2f( 0.0f, 1.0f ) );
}

// =============================================================================
// LayoutStyle
// =============================================================================

TEST_CASE( "LayoutStyle defaults have zero spacing", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.Spacing == 0.0f );
}

TEST_CASE( "LayoutStyle defaults to Overlay layout type", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.LayoutType == ELayoutType::Overlay );
}

TEST_CASE( "LayoutStyle defaults to TopLeft child alignment", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.ChildAlign == EAlignment::TopLeft );
}

TEST_CASE( "LayoutStyle defaults to NoWrap wrap mode", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.WrapMode == EWrapMode::NoWrap );
}

TEST_CASE( "LayoutStyle defaults to Flow positioning mode", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.PositionMode == EPositioningMode::Flow );
}

TEST_CASE( "LayoutStyle defaults to Content sizing modes", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.WidthMode  == ESizingMode::Content );
    REQUIRE( s.HeightMode == ESizingMode::Content );
}

TEST_CASE( "LayoutStyle defaults to Inherit self-alignment", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.SelfAlign == EAlignment::Inherit );
}

TEST_CASE( "LayoutStyle defaults to zero fixed sizes", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.FixedWidth  == 0.0f );
    REQUIRE( s.FixedHeight == 0.0f );
}

TEST_CASE( "LayoutStyle defaults to zero flex grow", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.FlexGrow == 0.0f );
}

TEST_CASE( "LayoutStyle defaults have zero padding and margin", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.Padding.Top    == 0.0f );
    REQUIRE( s.Padding.Right  == 0.0f );
    REQUIRE( s.Padding.Bottom == 0.0f );
    REQUIRE( s.Padding.Left   == 0.0f );
    REQUIRE( s.Margin.Top     == 0.0f );
    REQUIRE( s.Margin.Right   == 0.0f );
    REQUIRE( s.Margin.Bottom  == 0.0f );
    REQUIRE( s.Margin.Left    == 0.0f );
}

TEST_CASE( "LayoutStyle SizeConstraints defaults to Unbounded", "[layout][style]" )
{
    LayoutStyle s{};
    REQUIRE( s.SizeConstraints.MinSize[ 0 ] == 0.0f );
    REQUIRE( s.SizeConstraints.MinSize[ 1 ] == 0.0f );
    REQUIRE( s.SizeConstraints.MaxSize[ 0 ] == std::numeric_limits<f32>::max() );
    REQUIRE( s.SizeConstraints.MaxSize[ 1 ] == std::numeric_limits<f32>::max() );
}

// =============================================================================
// LayoutResult
// =============================================================================

TEST_CASE( "LayoutResult defaults to dirty", "[layout][result]" )
{
    LayoutResult r{};
    REQUIRE( r.IsDirty );
}

TEST_CASE( "LayoutResult defaults to zero desired size", "[layout][result]" )
{
    LayoutResult r{};
    RequireApproxEqual( r.DesiredSize, Vec2f( 0.0f, 0.0f ) );
}

TEST_CASE( "LayoutResult defaults to zero final rect", "[layout][result]" )
{
    LayoutResult r{};
    RequireApproxEqual( r.FinalRect.Min(), Vec2f( 0.0f, 0.0f ) );
    RequireApproxEqual( r.FinalRect.Max(), Vec2f( 0.0f, 0.0f ) );
}

TEST_CASE( "LayoutResult defaults to Visible visibility", "[layout][result]" )
{
    LayoutResult r{};
    REQUIRE( r.Visibility.Value == Visibility::Visible );
}

TEST_CASE( "LayoutResult IsDirty can be cleared", "[layout][result]" )
{
    LayoutResult r{};
    r.IsDirty = false;
    REQUIRE_FALSE( r.IsDirty );
}
