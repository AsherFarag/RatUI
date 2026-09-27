#include "TestFontCommon.h"

namespace
{
    /** @brief "Main" (regular + bold, 5 units per char at size 10) with a "Wide" fallback that alone has 'Z'. */
    struct RichTextFixture
    {
        FontLibrary      Fonts;
        FontFamilyHandle Main = Fonts.RegisterFamily( "Main"_id );
        FontFamilyHandle Wide = Fonts.RegisterFamily( "Wide"_id );
        FontFaceHandle   Regular, Bold, WideFace;
        TextMetrics      Metrics{ Fonts };

        RichTextFixture()
        {
            Regular  = AddFixedFace( Fonts, Main, {}, { EFontWeight::Regular } );
            Bold     = AddFixedFace( Fonts, Main, {}, { EFontWeight::Bold } );
            WideFace = AddFixedFace( Fonts, Wide, { .Characters = U"Z ", .AdvanceEm = 1.f, .Ascender = 1.6_fu, .Descender = -0.4_fu } );
            Fonts.SetFallbacks( Main, { Wide } );
        }

        TextLayoutStyle Style( TextWrap a_Wrap = TextWrap::WrapWord() ) const
        {
            return TextLayoutStyle{ .Family = Main, .Size = 10_u, .Wrap = a_Wrap };
        }

        Optional<PreparedText> Prepare( StringView a_Text, Span<const TextSpan> a_Spans = {}, TextLayoutStyle a_Style = {} )
        {
            if ( !a_Style.Family.IsValid() )
                a_Style = Style( a_Style.Wrap );
            return Metrics.Prepare( StyledTextView{ a_Text, a_Spans }, a_Style );
        }

        String LineText( const PreparedText& a_Prepared, const ShapedText& a_Shaped, u32 a_Line ) const
        {
            const ShapedLine& line = a_Shaped.Lines[a_Line];
            if ( line.Start == line.End )
                return {};
            const u32 first = a_Shaped.Glyphs[line.Start].Cluster;
            const u32 last  = a_Shaped.Glyphs[line.End - 1].Cluster;
            return String{ a_Prepared.NormalizedText.substr( first, last - first + 1 ) };
        }
    };
}

TEST_CASE( "Rich text itemizes spans into runs", "[Text][RichText]" )
{
    RichTextFixture f;

    SECTION( "Plain text is a single run" )
    {
        const auto prepared = f.Prepare( "hello world" );
        REQUIRE( prepared );
        REQUIRE( Size( prepared->Runs ) == 1 );
        REQUIRE( prepared->Runs[0].Face.Face == f.Regular );
        REQUIRE( prepared->Runs[0].EndByte == 11 );
    }

    SECTION( "Spans split runs and resolve their own face" )
    {
        const TextSpan spans[] = { { 6, 11, { .Weight = EFontWeight::Bold } } };
        const auto prepared = f.Prepare( "hello world", spans );
        REQUIRE( Size( prepared->Runs ) == 2 );
        REQUIRE( prepared->Runs[0].Face.Face == f.Regular );
        REQUIRE( prepared->Runs[1].Face.Face == f.Bold );
        REQUIRE( prepared->Runs[1].StartByte == 6 );
        REQUIRE_FALSE( prepared->Runs[1].Face.SynthBold );
    }

    SECTION( "Spans that change nothing are merged away" )
    {
        const TextSpan spans[] = { { 2, 4, { .Weight = EFontWeight::Regular } } };
        const auto prepared = f.Prepare( "hello", spans );
        REQUIRE( Size( prepared->Runs ) == 1 );
    }

    SECTION( "Overlapping spans merge, later spans win, decorations accumulate" )
    {
        const TextSpan spans[] = {
            { 0, 5, { .FillColor = Colors::Red, .Decorations = ETextDecoration::Underline } },
            { 2, 5, { .FillColor = Colors::Blue, .Decorations = ETextDecoration::Strikethrough } },
        };
        const auto prepared = f.Prepare( "hello", spans );
        REQUIRE( Size( prepared->Runs ) == 2 );
        REQUIRE( prepared->Runs[0].FillColor == Colors::Red );
        REQUIRE( prepared->Runs[1].FillColor == Colors::Blue );
        REQUIRE( prepared->Runs[1].Decorations == ( ETextDecoration::Underline | ETextDecoration::Strikethrough ) );
    }

    SECTION( "Span offsets are remapped through whitespace collapsing" )
    {
        // Source "a   b" (b at byte 4) normalizes to "a b" (b at byte 2).
        const TextSpan spans[] = { { 4, 5, { .Weight = EFontWeight::Bold } } };
        const auto prepared = f.Prepare( "a   b", spans );
        REQUIRE( prepared->NormalizedText == "a b" );
        REQUIRE( Size( prepared->Runs ) == 2 );
        REQUIRE( prepared->Runs[1].StartByte == 2 );
        REQUIRE( prepared->Runs[1].Face.Face == f.Bold );
    }

    SECTION( "Missing characters fall back to another family" )
    {
        const auto prepared = f.Prepare( "abZab" );
        REQUIRE( Size( prepared->Runs ) == 3 );
        REQUIRE( prepared->Runs[1].Face.Face == f.WideFace );
        REQUIRE( prepared->Runs[1].StartByte == 2 );
        REQUIRE( prepared->Runs[1].EndByte == 3 );
    }

    SECTION( "No resolvable font means no prepared text" )
    {
        FontLibrary empty;
        TextMetrics metrics{ empty };
        REQUIRE_FALSE( metrics.Prepare( "abc", TextLayoutStyle{} ) );
    }
}

TEST_CASE( "Rich text measures and breaks across runs", "[Text][RichText]" )
{
    RichTextFixture f;

    SECTION( "A word split across runs is one segment and never broken between the runs" )
    {
        // "xx word": "wo" bold, "rd" regular. Each character is 5 units wide.
        const TextSpan spans[] = { { 3, 5, { .Weight = EFontWeight::Bold } } };
        const auto prepared = f.Prepare( "xx word", spans );
        REQUIRE( Size( prepared->Segments ) == 3 );             // "xx", " ", "word"
        REQUIRE( prepared->Segments[2].Width == 20_u );

        const auto shaped = f.Metrics.Shape( *prepared, f.Style(), { 30_u, Limits<Unit>::max() } );
        REQUIRE( shaped );
        REQUIRE( shaped->LineCount() == 2 );
        REQUIRE( f.LineText( *prepared, *shaped, 1 ) == "word" );
        REQUIRE( Size( shaped->Runs ) == 3 );                     // "xx" | "wo" | "rd" (runs split at the line break too)
    }

    SECTION( "Segment widths add up to the shaped line width, including letter spacing" )
    {
        TextLayoutStyle style = f.Style();
        style.LetterSpacing = 1_u;
        const auto prepared = f.Prepare( "ab cd", {}, style );
        const auto shaped   = f.Metrics.Shape( *prepared, style );
        REQUIRE( shaped->Lines[0].Width.ToFloat() == Catch::Approx( 5 * 5.f + 5 * 1.f ) ); // 5 chars, spacing after each
        REQUIRE( prepared->Segments[0].Width == 12_u );
    }

    SECTION( "Synthetic bold adds advance" )
    {
        FontLibrary fonts;
        const FontFamilyHandle family = fonts.RegisterFamily( "Only"_id );
        AddFixedFace( fonts, family, {} );
        TextMetrics metrics{ fonts };

        const TextLayoutStyle regular{ .Family = family, .Size = 10_u, .Wrap = TextWrap::NoWrap() };
        TextLayoutStyle bold = regular;
        bold.Weight = EFontWeight::Bold;

        const auto regularShaped = metrics.Shape( *metrics.Prepare( "aaaa", regular ), regular );
        const auto boldShaped    = metrics.Shape( *metrics.Prepare( "aaaa", bold ), bold );
        REQUIRE( boldShaped->Lines[0].Width > regularShaped->Lines[0].Width );
    }
}

TEST_CASE( "Rich text line metrics follow the runs on each line", "[Text][RichText]" )
{
    RichTextFixture f;

    SECTION( "A line containing a taller run is taller" )
    {
        // Line 1 contains a big span; line 2 is regular text. Each word gets its own line.
        const TextSpan spans[] = { { 0, 3, { .Size = 20_u } } };
        const auto prepared = f.Prepare( "big small", spans );
        const auto shaped   = f.Metrics.Shape( *prepared, f.Style(), { 50_u, Limits<Unit>::max() } );
        REQUIRE( shaped->LineCount() == 2 );

        const ShapedLine& first  = shaped->Lines[0];
        const ShapedLine& second = shaped->Lines[1];
        REQUIRE( first.Height.ToFloat() == Catch::Approx( 20.f ) );  // (0.8 + 0.2) em * 20
        REQUIRE( second.Height.ToFloat() == Catch::Approx( 10.f ) );
        REQUIRE( first.Baseline.ToFloat() == Catch::Approx( 16.f ) );
        REQUIRE( second.Top.ToFloat() == Catch::Approx( 20.f ) );
        REQUIRE( second.Baseline.ToFloat() == Catch::Approx( 28.f ) );
        REQUIRE( shaped->TotalHeight.ToFloat() == Catch::Approx( 30.f ) );
    }

    SECTION( "Fallback runs contribute their own metrics" )
    {
        const auto prepared = f.Prepare( "aZa" );
        const auto shaped   = f.Metrics.Shape( *prepared, f.Style() );
        REQUIRE( shaped->Lines[0].Height.ToFloat() == Catch::Approx( 20.f ) ); // Wide face: (1.6 + 0.4) em * 10
    }

    SECTION( "Explicit line height centres the content" )
    {
        TextLayoutStyle style = f.Style();
        style.LineHeight = 20_u;
        const auto prepared = f.Prepare( "a", {}, style );
        const auto shaped   = f.Metrics.Shape( *prepared, style );
        REQUIRE( shaped->Lines[0].Height.ToFloat() == Catch::Approx( 20.f ) );
        REQUIRE( shaped->Lines[0].Baseline.ToFloat() == Catch::Approx( 5.f + 8.f ) );
    }
}

TEST_CASE( "Rich text snaps pixel faces to their grid", "[Text][RichText][Pixel]" )
{
    FontLibrary fonts;
    const FontFamilyHandle pixel = fonts.RegisterFamily( "Pixel"_id );
    // 8px native grid, advance 0.4375em = 3.5 native px (deliberately off-grid).
    AddFixedFace( fonts, pixel, { .AdvanceEm = 0.4375f, .Mode = EGlyphRenderMode::Pixel, .NativePixelSize = 8.f } );
    TextMetrics metrics{ fonts };

    const TextLayoutStyle style{ .Family = pixel, .Size = 20_u, .Wrap = TextWrap::NoWrap() };
    const auto prepared = metrics.Prepare( "ab", style );
    REQUIRE( prepared );

    SECTION( "Size snaps to a whole multiple of the native size" )
    {
        REQUIRE( prepared->Runs[0].Size.ToFloat() == Catch::Approx( 24.f ) ); // round( 20 / 8 ) = 3x
    }

    SECTION( "Advances snap to whole native pixels" )
    {
        const auto shaped = metrics.Shape( *prepared, style );
        const f32 nativeUnit = 24.f / 8.f;
        for ( const ShapedGlyph& glyph : shaped->Glyphs )
        {
            const f32 native = glyph.XAdvance.ToFloat() / nativeUnit;
            REQUIRE( native == Catch::Approx( std::round( native ) ) );
        }
    }
}

TEST_CASE( "Rich text ellipsis uses the style the line ends with", "[Text][RichText]" )
{
    RichTextFixture f;
    TextLayoutStyle style = f.Style( TextWrap::NoWrap() );
    style.Overflow = ETextOverflow::Ellipsis;

    const TextSpan spans[] = { { 4, 9, { .Weight = EFontWeight::Bold } } };
    const auto prepared = f.Prepare( "abc defghijkl", spans, style );
    const auto shaped   = f.Metrics.Shape( *prepared, style, { 40_u, Limits<Unit>::max() } );
    REQUIRE( shaped );
    REQUIRE( shaped->LineCount() == 1 );
    REQUIRE( shaped->Lines[0].Width <= 40_u );

    // "abc d" (25) + "..." (15, no U+2026) as its own regular run after the bold "d".
    REQUIRE( shaped->Lines[0].Width.ToFloat() == Catch::Approx( 40.f ) );
    REQUIRE( Size( shaped->Runs ) == 3 );
    REQUIRE( shaped->Runs[1].Face.Face == f.Bold );
    const ShapedRun& lastRun = Back( shaped->Runs );
    REQUIRE( lastRun.Face.Face == f.Regular );
    REQUIRE( lastRun.GlyphEnd - lastRun.GlyphStart == 3 );
}
