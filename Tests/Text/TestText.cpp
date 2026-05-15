/**
 * @file TestText.cpp
 * @brief Tests for RatUI text types and layout helpers.
 */

#include <RatUI/Text/Text.h>
#include <RatUI/Text/TextLayout.h>
#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <vector>

using namespace RatUI;

namespace
{
    Unit MeasureByCodepoint( StringView a_Text )
    {
        u32 count = 0;
        for ( Unicode::UTF8Iterator it( a_Text ); it; ++it )
            ++count;
        return Unit{ static_cast<f32>( count ) };
    }

    String SegmentText( const PreparedText& a_Prepared, const TextSegment& a_Segment )
    {
        return String{ Data( a_Prepared.NormalizedText ) + a_Segment.StartByte, a_Segment.ByteLength };
    }
}

TEST_CASE( "TextWrap presets and Prewrap state are correct", "[text][wrap]" )
{
    const TextWrap noWrap = TextWrap::NoWrap();
    REQUIRE( noWrap.BreakMode == EBreakMode::None );
    REQUIRE( noWrap.Whitespace == EWhitespace::Collapse );
    REQUIRE( noWrap.Newline == ENewline::Preserve );
    REQUIRE_FALSE( noWrap.Prewrap() );

    const TextWrap wrapWord = TextWrap::WrapWord();
    REQUIRE( wrapWord.BreakMode == EBreakMode::Word );
    REQUIRE( wrapWord.Whitespace == EWhitespace::Collapse );
    REQUIRE( wrapWord.Newline == ENewline::Preserve );
    REQUIRE( wrapWord.Prewrap() );

    const TextWrap wrapChar = TextWrap::WrapChar();
    REQUIRE( wrapChar.BreakMode == EBreakMode::Char );
    REQUIRE( wrapChar.Whitespace == EWhitespace::Collapse );
    REQUIRE( wrapChar.Newline == ENewline::Preserve );
    REQUIRE( wrapChar.Prewrap() );
}

TEST_CASE( "Prepare NoWrap collapses whitespace and emits a single text segment", "[text][prepare]" )
{
    const PreparedText prepared = TextLayout::Prepare( "  Hello\t\n  world  ", TextWrap::NoWrap(), MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText == "Hello world" );
    REQUIRE( prepared.HyphenWidth == 1_u );
    REQUIRE( Size( prepared.Segments ) == 1 );

    const TextSegment& seg = prepared.Segments[0];
    REQUIRE( seg.Kind == ESegmentKind::Text );
    REQUIRE_FALSE( seg.IsCJKChar );
    REQUIRE( seg.Width == 11_u );
    REQUIRE( seg.PaintWidth == 11_u );
    REQUIRE( SegmentText( prepared, seg ) == "Hello world" );
}

TEST_CASE( "Prepare WrapWord emits word, space, and CJK segments", "[text][prepare]" )
{
    const PreparedText prepared = TextLayout::Prepare( "Hello 世界 test", TextWrap::WrapWord(), MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText == "Hello 世界 test" );
    REQUIRE( Size( prepared.Segments ) == 6 );

    const TextSegment& seg0 = prepared.Segments[0];
    REQUIRE( seg0.Kind == ESegmentKind::Text );
    REQUIRE_FALSE( seg0.IsCJKChar );
    REQUIRE( SegmentText( prepared, seg0 ) == "Hello" );

    const TextSegment& seg1 = prepared.Segments[1];
    REQUIRE( seg1.Kind == ESegmentKind::Space );
    REQUIRE( seg1.Width == 1_u );
    REQUIRE( seg1.PaintWidth == 0_u );
    REQUIRE( SegmentText( prepared, seg1 ) == " " );

    const TextSegment& seg2 = prepared.Segments[2];
    REQUIRE( seg2.Kind == ESegmentKind::Text );
    REQUIRE( seg2.IsCJKChar );
    REQUIRE( SegmentText( prepared, seg2 ) == "世" );

    const TextSegment& seg3 = prepared.Segments[3];
    REQUIRE( seg3.Kind == ESegmentKind::Text );
    REQUIRE( seg3.IsCJKChar );
    REQUIRE( SegmentText( prepared, seg3 ) == "界" );

    const TextSegment& seg4 = prepared.Segments[4];
    REQUIRE( seg4.Kind == ESegmentKind::Space );
    REQUIRE( SegmentText( prepared, seg4 ) == " " );

    const TextSegment& seg5 = prepared.Segments[5];
    REQUIRE( seg5.Kind == ESegmentKind::Text );
    REQUIRE_FALSE( seg5.IsCJKChar );
    REQUIRE( SegmentText( prepared, seg5 ) == "test" );
}

TEST_CASE( "Prepare WrapChar emits one text segment per codepoint", "[text][prepare]" )
{
    const PreparedText prepared = TextLayout::Prepare( "ab世 c", TextWrap::WrapChar(), MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText == "ab世 c" );
    REQUIRE( Size( prepared.Segments ) == 5 );

    REQUIRE( SegmentText( prepared, prepared.Segments[0] ) == "a" );
    REQUIRE( prepared.Segments[0].Kind == ESegmentKind::Text );
    REQUIRE_FALSE( prepared.Segments[0].IsCJKChar );

    REQUIRE( SegmentText( prepared, prepared.Segments[1] ) == "b" );
    REQUIRE( prepared.Segments[1].Kind == ESegmentKind::Text );
    REQUIRE_FALSE( prepared.Segments[1].IsCJKChar );

    REQUIRE( SegmentText( prepared, prepared.Segments[2] ) == "世" );
    REQUIRE( prepared.Segments[2].Kind == ESegmentKind::Text );
    REQUIRE( prepared.Segments[2].IsCJKChar );

    REQUIRE( SegmentText( prepared, prepared.Segments[3] ) == " " );
    REQUIRE( prepared.Segments[3].Kind == ESegmentKind::Space );

    REQUIRE( SegmentText( prepared, prepared.Segments[4] ) == "c" );
    REQUIRE( prepared.Segments[4].Kind == ESegmentKind::Text );
}

TEST_CASE( "Prepare emits hard breaks for preserved newlines", "[text][prepare]" )
{
    const PreparedText prepared = TextLayout::Prepare( "a\r\nb", TextWrap::WrapWord(), MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText == "a\nb" );
    REQUIRE( Size( prepared.Segments ) == 3 );

    REQUIRE( prepared.Segments[0].Kind == ESegmentKind::Text );
    REQUIRE( SegmentText( prepared, prepared.Segments[0] ) == "a" );

    REQUIRE( prepared.Segments[1].Kind == ESegmentKind::HardBreak );
    REQUIRE( prepared.Segments[1].Width == 0_u );
    REQUIRE( prepared.Segments[1].PaintWidth == 0_u );
    REQUIRE( SegmentText( prepared, prepared.Segments[1] ) == "\n" );

    REQUIRE( prepared.Segments[2].Kind == ESegmentKind::Text );
    REQUIRE( SegmentText( prepared, prepared.Segments[2] ) == "b" );
}

TEST_CASE( "WalkLines wraps at spaces and reports paint width without trailing spaces", "[text][layout]" )
{
    const PreparedText prepared = TextLayout::Prepare( "one two three", TextWrap::WrapWord(), MeasureByCodepoint );

    std::vector<std::tuple<u32, u32, Unit>> lines;
    const u32 lineCount = TextLayout::WalkLines( prepared, 7_u, 0,
        [&]( u32 a_Start, u32 a_End, Unit a_PaintWidth )
        {
            lines.emplace_back( a_Start, a_End, a_PaintWidth );
        } );

    REQUIRE( lineCount == 2 );
    REQUIRE( lines.size() == 2 );

    REQUIRE( std::get<0>( lines[0] ) == 0 );
    REQUIRE( std::get<1>( lines[0] ) == 3 );
    REQUIRE( std::get<2>( lines[0] ) == 7_u );

    REQUIRE( std::get<0>( lines[1] ) == 4 );
    REQUIRE( std::get<1>( lines[1] ) == Size( prepared.Segments ) );
    REQUIRE( std::get<2>( lines[1] ) == 5_u );
}

TEST_CASE( "WalkLines respects max-lines limit", "[text][layout]" )
{
    const PreparedText prepared = TextLayout::Prepare( "one two three", TextWrap::WrapWord(), MeasureByCodepoint );

    u32 callbackCount = 0;
    const u32 lineCount = TextLayout::WalkLines( prepared, 7_u, 1,
        [&]( u32, u32, Unit )
        {
            ++callbackCount;
        } );

    REQUIRE( lineCount == 1 );
    REQUIRE( callbackCount == 1 );
}

TEST_CASE( "WalkLines emits separate lines around hard breaks", "[text][layout]" )
{
    const PreparedText prepared = TextLayout::Prepare( "a\nb", TextWrap::WrapWord(), MeasureByCodepoint );

    std::vector<std::tuple<u32, u32, Unit>> lines;
    const u32 lineCount = TextLayout::WalkLines( prepared, 10_u, 0,
        [&]( u32 a_Start, u32 a_End, Unit a_PaintWidth )
        {
            lines.emplace_back( a_Start, a_End, a_PaintWidth );
        } );

    REQUIRE( lineCount == 2 );
    REQUIRE( lines.size() == 2 );

    REQUIRE( std::get<0>( lines[0] ) == 0 );
    REQUIRE( std::get<1>( lines[0] ) == 1 );
    REQUIRE( std::get<2>( lines[0] ) == 1_u );

    REQUIRE( std::get<0>( lines[1] ) == 2 );
    REQUIRE( std::get<1>( lines[1] ) == 3 );
    REQUIRE( std::get<2>( lines[1] ) == 1_u );
}

TEST_CASE( "Prepare NoWrap returns empty result for whitespace-only input", "[text][prepare][edge]" )
{
    const PreparedText prepared = TextLayout::Prepare( " \t\n\r\f ", TextWrap::NoWrap(), MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText.empty() );
    REQUIRE( prepared.HyphenWidth == 0_u );
    REQUIRE( prepared.Segments.empty() );
}

TEST_CASE( "Prepare with collapsed newlines does not emit hard-break segments", "[text][prepare][edge]" )
{
    TextWrap wrap = TextWrap::WrapWord();
    wrap.Newline = ENewline::Collapse;

    const PreparedText prepared = TextLayout::Prepare( "a\r\nb", wrap, MeasureByCodepoint );

    REQUIRE( prepared.NormalizedText == "a b" );
    REQUIRE( Size( prepared.Segments ) == 3 );
    REQUIRE( prepared.Segments[0].Kind == ESegmentKind::Text );
    REQUIRE( prepared.Segments[1].Kind == ESegmentKind::Space );
    REQUIRE( prepared.Segments[2].Kind == ESegmentKind::Text );
}

TEST_CASE( "WalkLines skips leading space segments at the start of a line", "[text][layout][edge]" )
{
    const PreparedText prepared = TextLayout::Prepare( "   a", TextWrap::PreWrap(), MeasureByCodepoint );
    REQUIRE( Size( prepared.Segments ) == 2 );
    REQUIRE( prepared.Segments[0].Kind == ESegmentKind::Space );
    REQUIRE( prepared.Segments[1].Kind == ESegmentKind::Text );

    std::vector<std::tuple<u32, u32, Unit>> lines;
    const u32 lineCount = TextLayout::WalkLines( prepared, 10_u, 0,
        [&]( u32 a_Start, u32 a_End, Unit a_PaintWidth )
        {
            lines.emplace_back( a_Start, a_End, a_PaintWidth );
        } );

    REQUIRE( lineCount == 1 );
    REQUIRE( lines.size() == 1 );
    REQUIRE( std::get<0>( lines[0] ) == 1 );
    REQUIRE( std::get<1>( lines[0] ) == 2 );
    REQUIRE( std::get<2>( lines[0] ) == 1_u );
}

TEST_CASE( "WalkLines returns zero lines for empty prepared text", "[text][layout][edge]" )
{
    const PreparedText prepared = TextLayout::Prepare( "", TextWrap::WrapWord(), MeasureByCodepoint );

    u32 callbackCount = 0;
    const u32 lineCount = TextLayout::WalkLines( prepared, 5_u, 0,
        [&]( u32, u32, Unit )
        {
            ++callbackCount;
        } );

    REQUIRE( lineCount == 0 );
    REQUIRE( callbackCount == 0 );
}

TEST_CASE( "WalkLines with zero width emits one wrap-char segment per line", "[text][layout][edge]" )
{
    const PreparedText prepared = TextLayout::Prepare( "ab", TextWrap::WrapChar(), MeasureByCodepoint );

    std::vector<std::tuple<u32, u32, Unit>> lines;
    const u32 lineCount = TextLayout::WalkLines( prepared, 0_u, 0,
        [&]( u32 a_Start, u32 a_End, Unit a_PaintWidth )
        {
            lines.emplace_back( a_Start, a_End, a_PaintWidth );
        } );

    REQUIRE( lineCount == 2 );
    REQUIRE( lines.size() == 2 );
    REQUIRE( std::get<0>( lines[0] ) == 0 );
    REQUIRE( std::get<1>( lines[0] ) == 1 );
    REQUIRE( std::get<2>( lines[0] ) == 1_u );
    REQUIRE( std::get<0>( lines[1] ) == 1 );
    REQUIRE( std::get<1>( lines[1] ) == 2 );
    REQUIRE( std::get<2>( lines[1] ) == 1_u );
}
