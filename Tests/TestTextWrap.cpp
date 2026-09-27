#include "TestFontCommon.h"

namespace
{
    String Normalize( StringView a_Text, TextWrap a_Wrap, Array<u32>* o_Map = nullptr )
    {
        return TextLayout::NormalizeText( a_Text, a_Wrap, o_Map );
    }

    /** @brief Monospace test layout: every character (including spaces) is 0.5em = 5 units at size 10. */
    struct WrapFixture
    {
        FontLibrary      Fonts;
        FontFamilyHandle Family = Fonts.RegisterFamily( "Mono"_id );
        TextMetrics      Metrics{ Fonts };

        WrapFixture() { AddFixedFace( Fonts, Family, { .Characters = U" \tabcdefghijklmnopqrstuvwxyz" } ); }

        /** @brief Lays out @p a_Text and returns the text of each line. */
        Array<String> Lines( StringView a_Text, TextWrap a_Wrap, Unit a_MaxWidth = Limits<Unit>::max() )
        {
            const TextLayoutStyle style{ .Family = Family, .Size = 10_u, .Wrap = a_Wrap };
            const auto prepared = Metrics.Prepare( a_Text, style );
            REQUIRE( prepared );
            const auto shaped = Metrics.Shape( *prepared, style, { a_MaxWidth, Limits<Unit>::max() } );
            REQUIRE( shaped );

            Array<String> lines;
            for ( const ShapedLine& line : shaped->Lines )
            {
                if ( line.Start == line.End )
                {
                    PushBack( lines, String{} );
                    continue;
                }
                const u32 first = shaped->Glyphs[line.Start].Cluster;
                const u32 last  = shaped->Glyphs[line.End - 1].Cluster;
                PushBack( lines, String{ prepared->NormalizedText.substr( first, last - first + 1 ) } );
            }
            return lines;
        }
    };
}

TEST_CASE( "Whitespace normalisation follows each TextWrap preset", "[Text][Whitespace]" )
{
    const StringView text = "  one   two \r\n\t three\n\nfour  ";

    SECTION( "NoWrap collapses everything, newlines included (CSS nowrap)" )
    {
        REQUIRE( Normalize( text, TextWrap::NoWrap() ) == "one two three four" );
    }

    SECTION( "WrapWord collapses spaces but keeps newlines, trimming around them (CSS pre-line)" )
    {
        REQUIRE( Normalize( text, TextWrap::WrapWord() ) == "one two\nthree\n\nfour" );
        REQUIRE( Normalize( text, TextWrap::WrapChar() ) == "one two\nthree\n\nfour" );
        REQUIRE( Normalize( text, TextWrap::PreLine() ) == "one two\nthree\n\nfour" );
    }

    SECTION( "Pre and PreWrap keep spaces and tabs, and normalise line endings (CSS pre / pre-wrap)" )
    {
        REQUIRE( Normalize( text, TextWrap::Pre() ) == "  one   two \n\t three\n\nfour  " );
        REQUIRE( Normalize( text, TextWrap::PreWrap() ) == "  one   two \n\t three\n\nfour  " );
    }

    SECTION( "Preserved spaces with collapsed newlines turn each newline into a space" )
    {
        const TextWrap wrap{ EBreakMode::Word, EWhitespace::Preserve, ENewline::Collapse };
        REQUIRE( Normalize( "a \r\nb", wrap ) == "a  b" );
    }

    SECTION( "Unicode line separators are newlines" )
    {
        REQUIRE( Normalize( "a\xC2\x85" "b\xE2\x80\xA8" "c\xE2\x80\xA9" "d", TextWrap::WrapWord() ) == "a\nb\nc\nd" );
        REQUIRE( Normalize( "a\xE2\x80\xA8" "b", TextWrap::NoWrap() ) == "a b" );
    }

    SECTION( "The offset map points every source byte at its normalised position" )
    {
        Array<u32> map;
        const String result = Normalize( "  ab   cd ", TextWrap::WrapWord(), &map );
        REQUIRE( result == "ab cd" );
        REQUIRE( Size( map ) == 11 );
        REQUIRE( map[0] == 0 );  // Leading spaces map to the first character.
        REQUIRE( map[2] == 0 );  // 'a'
        REQUIRE( map[4] == 2 );  // Collapsed run -> the single space.
        REQUIRE( map[7] == 3 );  // 'c'
        REQUIRE( map[9] == 5 );  // Trailing space -> end.
        REQUIRE( map[10] == 5 ); // End of text.
    }
}

TEST_CASE( "Line breaking honours newlines and wrapping independently", "[Text][Whitespace]" )
{
    WrapFixture f;

    SECTION( "WrapWord breaks at newlines and keeps empty lines" )
    {
        const auto lines = f.Lines( "one  two\n\nthree", TextWrap::WrapWord() );
        REQUIRE( Size( lines ) == 3 );
        REQUIRE( lines[0] == "one two" );
        REQUIRE( lines[1] == "" );
        REQUIRE( lines[2] == "three" );
    }

    SECTION( "NoWrap stays on one line even with newlines" )
    {
        const auto lines = f.Lines( "one\ntwo", TextWrap::NoWrap(), 10_u );
        REQUIRE( Size( lines ) == 1 );
        REQUIRE( lines[0] == "one two" );
    }

    SECTION( "Pre breaks only at newlines, never to fit the width, and keeps spaces" )
    {
        const auto lines = f.Lines( "a  b c d e f\n  g", TextWrap::Pre(), 20_u );
        REQUIRE( Size( lines ) == 2 );
        REQUIRE( lines[0] == "a  b c d e f" );
        REQUIRE( lines[1] == "  g" );
    }

    SECTION( "PreWrap keeps indentation after hard breaks but hangs spaces at soft wraps" )
    {
        // Width fits 6 characters (30 units).
        const auto lines = f.Lines( "  abcd\n    ab cd ef", TextWrap::PreWrap(), 30_u );
        REQUIRE( Size( lines ) == 3 );
        REQUIRE( lines[0] == "  abcd" );
        REQUIRE( lines[1] == "    ab" );
        REQUIRE( lines[2] == "cd ef" );
    }

    SECTION( "PreWrap never swallows a newline into a trailing space run" )
    {
        const auto lines = f.Lines( "ab  \ncd", TextWrap::PreWrap() );
        REQUIRE( Size( lines ) == 2 );
        REQUIRE( lines[1] == "cd" );
    }
}
