#include <catch2/catch_test_macros.hpp>
#include <RatUI/Text/TextLayout.h>

using namespace RatUI;

namespace
{
    struct Line
    {
        u32  Start, End;
        Unit Width;
    };

    /** @brief Lays out a_Text with every byte 10 wide. */
    Array<Line> LayOut( StringView a_Text, TextWrap a_Wrap, Unit a_MaxWidth = Limits<Unit>::max() )
    {
        const PreparedText prepared = TextLayout::Prepare( a_Text, a_Wrap, []( StringView a_Run ) { return Unit{ 10.f * static_cast<f32>( Size( a_Run ) ) }; } );

        Array<Line> lines;
        TextLayout::WalkLines( prepared, a_MaxWidth, 0u, [&]( u32 a_Start, u32 a_End, Unit a_Width ) { PushBack( lines, Line{ a_Start, a_End, a_Width } ); } );
        return lines;
    }
}

TEST_CASE( "TextLayout: preserved whitespace keeps indentation after a newline", "[Text]" )
{
    const Array<Line> lines = LayOut( "ab\n   cd", TextWrap::PreWrap() );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[1].Width.ToFloat() == 50.f ); // "   cd"
}

TEST_CASE( "TextLayout: a line of only preserved spaces is still a line", "[Text]" )
{
    // The caret after the spaces is measured on such a line.
    const Array<Line> lines = LayOut( "ab\n   ", TextWrap::PreWrap() );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[1].End > lines[1].Start );
}

TEST_CASE( "TextLayout: collapsed whitespace still drops leading spaces", "[Text]" )
{
    const Array<Line> lines = LayOut( "ab\n   cd", TextWrap::WrapWord() );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[1].Width.ToFloat() == 20.f ); // "cd"
}

TEST_CASE( "TextLayout: preserved whitespace still drops spaces at a soft wrap", "[Text]" )
{
    const Array<Line> lines = LayOut( "aaaa bbbb", TextWrap::PreWrap(), 50_u );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[1].Width.ToFloat() == 40.f ); // "bbbb"
}

TEST_CASE( "TextLayout: Pre keeps spaces and breaks lines only at line breaks", "[Text]" )
{
    const PreparedText prepared = TextLayout::Prepare( "a  b\n  c", TextWrap::Pre(), []( StringView a_Run ) { return Unit{ 10.f * static_cast<f32>( Size( a_Run ) ) }; } );
    CHECK( prepared.NormalizedText == "a  b\n  c" );

    const Array<Line> lines = LayOut( "a  b\n  c", TextWrap::Pre(), 20_u ); // Narrower than either line: no wrapping.
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[0].Width.ToFloat() == 40.f );
    CHECK( lines[1].Width.ToFloat() == 30.f );
}

TEST_CASE( "TextLayout: collapsing modes collapse runs of spaces, and NoWrap collapses line breaks too", "[Text]" )
{
    const auto measure = []( StringView a_Run ) { return Unit{ 10.f * static_cast<f32>( Size( a_Run ) ) }; };
    CHECK( TextLayout::Prepare( "a    b \n  c", TextWrap::WrapWord(), measure ).NormalizedText == "a b\nc" );
    CHECK( TextLayout::Prepare( " a  \n b ", TextWrap::NoWrap(), measure ).NormalizedText == "a b" );
    CHECK( TextLayout::Prepare( "a \t b\r\nc", TextWrap::PreWrap(), measure ).NormalizedText == "a \t b\nc" );
}

TEST_CASE( "TextLayout: WrapChar breaks inside words and still collapses spaces", "[Text]" )
{
    const auto measure = []( StringView a_Run ) { return Unit{ 10.f * static_cast<f32>( Size( a_Run ) ) }; };
    CHECK( TextLayout::Prepare( "a   b", TextWrap::WrapChar(), measure ).NormalizedText == "a b" );

    const Array<Line> lines = LayOut( "abcdef", TextWrap::WrapChar(), 30_u );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[0].Width.ToFloat() == 30.f );
    CHECK( lines[1].Width.ToFloat() == 30.f );
}

TEST_CASE( "TextLayout: the whitespace mode applies whatever the break mode", "[Text]" )
{
    const auto measure = []( StringView a_Run ) { return Unit{ 10.f * static_cast<f32>( Size( a_Run ) ) }; };

    // Spaces kept while breaking by character.
    CHECK( TextLayout::Prepare( "a   b", TextWrap{ EBreakMode::Char, EWhitespace::Preserve, ENewline::Preserve }, measure ).NormalizedText == "a   b" );

    // Spaces kept, line breaks turned into spaces.
    CHECK( TextLayout::Prepare( "a  \nb", TextWrap{ EBreakMode::Word, EWhitespace::Preserve, ENewline::Collapse }, measure ).NormalizedText == "a   b" );

    // PreLine: spaces collapse, line breaks stay.
    CHECK( TextLayout::Prepare( "a   b\n  c", TextWrap::PreLine(), measure ).NormalizedText == "a b\nc" );
}

TEST_CASE( "TextLayout: an empty line between two line breaks is kept", "[Text]" )
{
    const Array<Line> lines = LayOut( "a\n\nb", TextWrap::Pre() );
    REQUIRE( Size( lines ) == 3 );
    CHECK( lines[1].Start == lines[1].End );
    CHECK( lines[1].Width.ToFloat() == 0.f );
}

TEST_CASE( "TextLayout: a word kept with the punctuation after it wraps with it", "[Text]" )
{
    // The full stop (30 wide) can't start a line, so "world" moves down with it instead of being dropped.
    const Array<Line> lines = LayOut( "hello world\xE3\x80\x82", TextWrap::WrapWord(), 110_u );
    REQUIRE( Size( lines ) == 2 );
    CHECK( lines[0].Width.ToFloat() == 50.f );
    CHECK( lines[1].Start == lines[0].End );
    CHECK( lines[1].Width.ToFloat() == 80.f );
}
