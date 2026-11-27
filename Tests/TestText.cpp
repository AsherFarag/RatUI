/**
 * @file TestText.cpp
 * @brief Unicode helpers (whitespace normalisation, UTF-8 encoding, line breaks) and ShapedText's pure metrics
 *        (line boxes, baselines, caret offsets). No text backend needed.
 */

#include "TestCommon.h"
#include <RatUI/Text/Text.h>

// ============================================================================
// Whitespace normalisation
// ============================================================================

TEST_CASE( "NormalizeWhitespace: collapses runs and trims the ends", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespace( "  Hello \t\n  World  " ) == "Hello World" );
    REQUIRE( Unicode::NormalizeWhitespace( "NoSpaces" ) == "NoSpaces" );
    REQUIRE( Unicode::NormalizeWhitespace( " \r\n\t " ).empty() );
}

TEST_CASE( "NormalizeWhitespace: Unicode line separators are whitespace", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespace( "A\xE2\x80\xA8" "B\xE2\x80\xA9" "C\xC2\x85" "D" ) == "A B C D" );
}

TEST_CASE( "NormalizeWhitespacePreWrap: keeps spaces and normalises line endings", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespacePreWrap( "  A  B  " ) == "  A  B  " );
    REQUIRE( Unicode::NormalizeWhitespacePreWrap( "A\r\nB\rC\fD" ) == "A\nB\nC\nD" );
    REQUIRE( Unicode::NormalizeWhitespacePreWrap( "A\xC2\x85" "B\xE2\x80\xA8" "C" ) == "A\nB\nC" );
}

TEST_CASE( "NormalizeWhitespace: spaces and line breaks are handled separately", "[Text]" )
{
    // pre-line: spaces collapse, line breaks stay, and spaces next to a break go.
    REQUIRE( Unicode::NormalizeWhitespace( "a   b  \n  c", true, true ) == "a b\nc" );

    // Spaces kept, line breaks turned into one space each ("\r\n" is one break).
    REQUIRE( Unicode::NormalizeWhitespace( "a\r\nb  c", false, false ) == "a b  c" );
    REQUIRE( Unicode::NormalizeWhitespace( "a\n\nb", false, false ) == "a  b" );
}

// ============================================================================
// UTF-8
// ============================================================================

TEST_CASE( "EncodeUTF8: one to four bytes per codepoint", "[Text][Unicode]" )
{
    const codepoint text[] = { U'A', codepoint{ 0xE9 }, codepoint{ 0x20AC }, U'\U0001F600' };
    REQUIRE( Unicode::EncodeUTF8( text, 4 ) == "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80" );
    REQUIRE( Unicode::EncodeUTF8( text, 0 ).empty() );
}

TEST_CASE( "EncodeUTF8: surrogates and values past U+10FFFF become U+FFFD", "[Text][Unicode]" )
{
    const codepoint invalid[] = { 0xD800, 0x110000 };
    REQUIRE( Unicode::EncodeUTF8( invalid, 2 ) == "\xEF\xBF\xBD\xEF\xBF\xBD" );
}

TEST_CASE( "UTF8Range: decodes multi-byte sequences and reports their byte positions", "[Text][Unicode]" )
{
    Array<codepoint> codepoints;
    Array<size>      bytes;
    for ( Unicode::UTF8Iterator it{ "a\xC3\xA9\xE2\x82\xAC" }; it; ++it )
    {
        PushBack( codepoints, *it );
        PushBack( bytes, it.ByteIndex() );
    }

    REQUIRE( codepoints == Array<codepoint>{ U'a', codepoint{ 0xE9 }, codepoint{ 0x20AC } } );
    REQUIRE( bytes == Array<size>{ 0, 1, 3 } );
}

TEST_CASE( "IsWhitespaceCluster: reads the codepoint at a byte offset", "[Text][Unicode]" )
{
    REQUIRE( Unicode::IsWhitespaceCluster( "a b", 1 ) );
    REQUIRE_FALSE( Unicode::IsWhitespaceCluster( "a b", 0 ) );
    REQUIRE( Unicode::IsWhitespaceCluster( "a\xE2\x80\xA8", 1 ) );
    REQUIRE_FALSE( Unicode::IsWhitespaceCluster( "a", 5 ) );
}

static_assert( Unicode::IsLineBreak( U'\n' ) && Unicode::IsLineBreak( U'\r' ) && Unicode::IsLineBreak( U'\f' ) );
static_assert( Unicode::IsLineBreak( 0x85 ) && Unicode::IsLineBreak( 0x2028 ) && Unicode::IsLineBreak( 0x2029 ) );
static_assert( !Unicode::IsLineBreak( U' ' ) && !Unicode::IsLineBreak( U'\t' ) );


// ============================================================================
// ShapedText metrics
// ============================================================================

namespace
{
    /** @brief Font size 10, so an XAdvance of 1 is 10 units wide. */
    ShapedText MakeShaped( std::initializer_list<ShapedGlyph> a_Glyphs, u32 a_TextEnd )
    {
        ShapedText text;
        text.FontSize = 10_u;
        for ( const ShapedGlyph& glyph : a_Glyphs )
            PushBack( text.Glyphs, glyph );
        PushBack( text.Lines, ShapedLine{ .Start = 0, .End = static_cast<u32>( Size( text.Glyphs ) ), .TextEnd = a_TextEnd } );
        return text;
    }
}

TEST_CASE( "ShapedText: line boxes stack with spacing only between lines", "[Text]" )
{
    ShapedText text;
    text.LineHeight  = 20_u;
    text.LineSpacing = 5_u;

    REQUIRE( text.BlockHeight( 0 ).ToFloat() == Catch::Approx( 0.f ) );
    REQUIRE( text.BlockHeight( 1 ).ToFloat() == Catch::Approx( 20.f ) );
    REQUIRE( text.BlockHeight( 3 ).ToFloat() == Catch::Approx( 70.f ) );
    REQUIRE( text.LineAdvance().ToFloat() == Catch::Approx( 25.f ) );

    REQUIRE( text.TotalHeight().ToFloat() == Catch::Approx( 0.f ) ); // No lines yet.
    PushBack( text.Lines, ShapedLine{} );
    PushBack( text.Lines, ShapedLine{} );
    REQUIRE( text.TotalHeight().ToFloat() == Catch::Approx( 45.f ) );
}

TEST_CASE( "ShapedText: the first baseline centres the glyphs in the line box", "[Text]" )
{
    ShapedText text;
    text.Ascender  = 12_u;
    text.Descender = Unit{ -4.f };

    text.LineHeight = 20_u; // 4 units of leading: half above.
    REQUIRE( text.FirstBaseline().ToFloat() == Catch::Approx( 14.f ) );

    text.LineHeight = 0_u; // No line height: the box is the ascent plus the descent.
    REQUIRE( text.FirstBaseline().ToFloat() == Catch::Approx( 12.f ) );
}

TEST_CASE( "ShapedText::CaretOffset: sums the advances of the clusters before the caret", "[Text]" )
{
    // "abc", one glyph per byte, 10 units each.
    const ShapedText text = MakeShaped( { { .XAdvance = FontUnit{ 1.f }, .Cluster = 0 },
                                          { .XAdvance = FontUnit{ 1.f }, .Cluster = 1 },
                                          { .XAdvance = FontUnit{ 1.f }, .Cluster = 2 } }, 3 );

    REQUIRE( text.CaretOffset( 0, 0 ).ToFloat() == Catch::Approx( 0.f ) );
    REQUIRE( text.CaretOffset( 0, 2 ).ToFloat() == Catch::Approx( 20.f ) );
    REQUIRE( text.CaretOffset( 0, 3 ).ToFloat() == Catch::Approx( 30.f ) );
    REQUIRE( text.CaretOffset( 0, 99 ).ToFloat() == Catch::Approx( 30.f ) );
    REQUIRE( text.CaretOffset( 5, 2 ).ToFloat() == Catch::Approx( 0.f ) ); // No such line.
}

TEST_CASE( "ShapedText::CaretOffset: a caret inside a multi-character cluster is placed in proportion", "[Text]" )
{
    // One 40-unit glyph (a ligature) for 4 bytes of text.
    const ShapedText text = MakeShaped( { { .XAdvance = FontUnit{ 4.f }, .Cluster = 0 } }, 4 );

    REQUIRE( text.CaretOffset( 0, 1 ).ToFloat() == Catch::Approx( 10.f ) );
    REQUIRE( text.CaretOffset( 0, 2 ).ToFloat() == Catch::Approx( 20.f ) );
    REQUIRE( text.CaretOffset( 0, 4 ).ToFloat() == Catch::Approx( 40.f ) );
}

TEST_CASE( "ShapedText::CaretOffset: several glyphs in one cluster all count once the caret passes it", "[Text]" )
{
    // A base letter and a combining mark share cluster 0 (2 bytes), then 'x'.
    const ShapedText text = MakeShaped( { { .XAdvance = FontUnit{ 1.f }, .Cluster = 0 },
                                          { .XAdvance = FontUnit{ 0.f }, .Cluster = 0 },
                                          { .XAdvance = FontUnit{ 1.f }, .Cluster = 2 } }, 3 );

    REQUIRE( text.CaretOffset( 0, 2 ).ToFloat() == Catch::Approx( 10.f ) );
    REQUIRE( text.CaretOffset( 0, 3 ).ToFloat() == Catch::Approx( 20.f ) );
}
