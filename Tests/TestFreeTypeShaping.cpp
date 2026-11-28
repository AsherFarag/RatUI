#if RATUI_BACKEND_FREETYPE

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <RatUI/Backends/FreeType/FontCache.h>
#include <RatUI/Backends/FreeType/TextMetrics.h>
#include <filesystem>

#ifndef RATUI_SOURCE_DIR
    #error "RATUI_SOURCE_DIR is set by Tests/CMakeLists.txt."
#endif

using namespace RatUI;

TEST_CASE( "FreeType shaping: blank lines have no glyphs", "[Text][FreeType]" )
{
    const std::filesystem::path font = std::filesystem::path( RATUI_SOURCE_DIR ) / "Examples/Resources/Fonts/Roboto-Medium.ttf";

    FreeType::FontCache cache;
    REQUIRE( cache.RegisterFontHandle( FontHandle{ 1 }, font.string() ) );
    FreeType::TextMetrics metrics{ cache };

    TextLayoutStyle style{};
    style.Font = FontHandle{ 1 };
    style.Size = 16_u;
    style.Wrap = TextWrap::WrapWord();

    // Enter pressed twice after "abc": the lines after the first must stay empty, not repeat "abc".
    const Optional<PreparedText> prepared = metrics.Prepare( "abc\n\nd", style );
    REQUIRE( prepared );
    const Optional<ShapedText> shaped = metrics.Shape( *prepared, style );
    REQUIRE( shaped );

    REQUIRE( Size( shaped->Lines ) == 3 );
    CHECK( shaped->Lines[0].End - shaped->Lines[0].Start == 3 );
    CHECK( shaped->Lines[1].End == shaped->Lines[1].Start );
    CHECK( shaped->Lines[2].End - shaped->Lines[2].Start == 1 );
    CHECK( Size( shaped->Glyphs ) == 4 );
}

TEST_CASE( "FreeType shaping: glyphs carry their clusters, and trailing spaces are shaped but hang", "[Text][FreeType]" )
{
    const std::filesystem::path font = std::filesystem::path( RATUI_SOURCE_DIR ) / "Examples/Resources/Fonts/Roboto-Medium.ttf";

    FreeType::FontCache cache;
    REQUIRE( cache.RegisterFontHandle( FontHandle{ 1 }, font.string() ) );
    FreeType::TextMetrics metrics{ cache };

    TextLayoutStyle style{};
    style.Font = FontHandle{ 1 };
    style.Size = 16_u;
    style.Wrap = TextWrap::PreWrap();

    const Optional<PreparedText> prepared = metrics.Prepare( "ab  \ncd", style );
    REQUIRE( prepared );
    const Optional<ShapedText> shaped = metrics.Shape( *prepared, style );
    REQUIRE( shaped );
    REQUIRE( shaped->LineCount() == 2 );

    const ShapedLine& first = shaped->Lines[0];
    CHECK( first.TextStart == 0 );
    CHECK( first.TextEnd == 4 );
    CHECK( shaped->Lines[1].TextStart == 5 );
    CHECK( shaped->Glyphs[shaped->Lines[1].Start].Cluster == 5 );

    // The caret moves through the spaces, past the line's visible width.
    const f32 afterB      = shaped->CaretOffset( 0, 2 ).ToFloat();
    const f32 afterSpaces = shaped->CaretOffset( 0, 4 ).ToFloat();
    CHECK( afterB == Catch::Approx( first.Width.ToFloat() ) );
    CHECK( afterSpaces > afterB );
    CHECK( shaped->CaretOffset( 0, 1 ).ToFloat() > 0.f );
    CHECK( shaped->CaretOffset( 0, 1 ).ToFloat() < afterB );
}

TEST_CASE( "FreeType shaping: hanging spaces don't change a line's width or spacing", "[Text][FreeType]" )
{
    const std::filesystem::path font = std::filesystem::path( RATUI_SOURCE_DIR ) / "Examples/Resources/Fonts/Roboto-Medium.ttf";

    FreeType::FontCache cache;
    REQUIRE( cache.RegisterFontHandle( FontHandle{ 1 }, font.string() ) );
    FreeType::TextMetrics metrics{ cache };

    TextLayoutStyle style{};
    style.Font          = FontHandle{ 1 };
    style.Size          = 16_u;
    style.Wrap          = TextWrap::PreWrap();
    style.LetterSpacing = 3_u;

    const Optional<ShapedText> withSpaces = metrics.Shape( *metrics.Prepare( "ab  ", style ), style );
    const Optional<ShapedText> plain      = metrics.Shape( *metrics.Prepare( "ab", style ), style );
    REQUIRE( withSpaces );
    REQUIRE( plain );
    CHECK( withSpaces->Lines[0].Width.ToFloat() == Catch::Approx( plain->Lines[0].Width.ToFloat() ) );
    CHECK( withSpaces->Lines[0].VisibleEnd == 2 );
    CHECK( withSpaces->Lines[0].TextEnd == 4 );
}

#endif
