#include "TestFontCommon.h"

#if RATUI_BACKEND_FREETYPE
#include <RatUI/Backends/FreeType/FontLoader.h>

namespace
{
    constexpr const char* c_Roboto    = RATUI_TEST_FONTS_DIR "/Roboto-Medium.ttf";
    constexpr const char* c_Minecraft = RATUI_TEST_FONTS_DIR "/Minecraft.ttf";

    GlyphBitmap Rasterize( IFontFace& a_Face, codepoint a_Codepoint, EGlyphRenderMode a_Mode, u16 a_PixelSize = 0, bool a_Bold = false )
    {
        GlyphBitmap bitmap;
        const GlyphRasterRequest request{ .Glyph = a_Face.GetGlyphIndex( a_Codepoint ), .Mode = a_Mode, .PixelSize = a_PixelSize, .SynthBold = a_Bold };
        REQUIRE( a_Face.RasterizeGlyph( request, bitmap ) );
        return bitmap;
    }
}

TEST_CASE( "FreeType faces load with sane metrics", "[Font][FreeType]" )
{
    FreeType::FontLoader loader;
    REQUIRE( loader.IsValid() );

    Unique<IFontFace> face = loader.LoadFromFile( c_Roboto );
    REQUIRE( face );
    REQUIRE( face->Kind() == EFontFaceKind::FreeType );
    REQUIRE( String{ face->GetDebugName() } == "Roboto Medium" );

    const FontFaceMetrics& metrics = face->Metrics();
    REQUIRE( metrics.Ascender > 0.5_fu );
    REQUIRE( metrics.Descender < 0_fu );
    REQUIRE( metrics.LineHeight() > 1_fu );
    REQUIRE( metrics.XHeight < metrics.CapHeight );
    REQUIRE( metrics.UnderlinePosition < 0_fu );

    REQUIRE( face->HasGlyph( U'A' ) );
    REQUIRE_FALSE( face->HasGlyph( U'\u3042' ) );

    SECTION( "Missing files fail cleanly" )
    {
        REQUIRE_FALSE( loader.LoadFromFile( RATUI_TEST_FONTS_DIR "/DoesNotExist.ttf" ) );
    }

    SECTION( "Faces outlive the loader" )
    {
        Unique<IFontFace> survivor;
        {
            FreeType::FontLoader temporary;
            survivor = temporary.LoadFromFile( c_Roboto );
        }
        REQUIRE( Rasterize( *survivor, U'H', EGlyphRenderMode::Raster, 16 ).Width > 0 );
    }
}

TEST_CASE( "FreeType shaping matches the cmap advances for simple text", "[Font][FreeType]" )
{
    FreeType::FontLoader loader;
    Unique<IFontFace> face = loader.LoadFromFile( c_Roboto );

    Array<ShapedGlyph> shaped;
    face->ShapeRun( "Hi", ShapeRunParams{ .Size = 100_u, .ClusterBase = 10 }, shaped );
    REQUIRE( Size( shaped ) == 2 );
    REQUIRE( shaped[0].Cluster == 10 );
    REQUIRE( shaped[1].Cluster == 11 );
    REQUIRE( shaped[0].XAdvance.ToFloat() == Catch::Approx( face->GetAdvance( shaped[0].GlyphIndex ).ToFloat() * 100.f ).margin( 0.5 ) );
}

TEST_CASE( "FreeType faces rasterize in every render mode", "[Font][FreeType]" )
{
    FreeType::FontLoader loader;

    SECTION( "MTSDF" )
    {
        Unique<IFontFace> face = loader.LoadFromFile( c_Roboto );
        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = face->GetGlyphIndex( U'H' ), .Mode = EGlyphRenderMode::MTSDF, .PixelSize = 32, .SDFPixelRange = 8.f }, bitmap ) );
        REQUIRE( bitmap.Width > 16 );               // Includes the SDF padding.
        REQUIRE( bitmap.Bearing[1] > 0.5f );        // em, above the baseline.
    }

    SECTION( "Raster glyph height follows the pixel size" )
    {
        Unique<IFontFace> face = loader.LoadFromFile( c_Roboto, { .Mode = EGlyphRenderMode::Raster } );
        const GlyphBitmap small = Rasterize( *face, U'H', EGlyphRenderMode::Raster, 16 );
        const GlyphBitmap large = Rasterize( *face, U'H', EGlyphRenderMode::Raster, 32 );
        REQUIRE( small.Height >= 10 );
        REQUIRE( small.Height <= 13 );
        REQUIRE( large.Height >= small.Height * 2 - 2 ); // Hinting rounds heights to whole pixels.

        const GlyphBitmap bold = Rasterize( *face, U'l', EGlyphRenderMode::Raster, 32, true );
        const GlyphBitmap thin = Rasterize( *face, U'l', EGlyphRenderMode::Raster, 32 );
        REQUIRE( bold.Width > thin.Width );
    }
}

TEST_CASE( "Pixel TTFs are detected and rasterized on their design grid", "[Font][FreeType][Pixel]" )
{
    FreeType::FontLoader loader;
    Unique<IFontFace> face = loader.LoadFromFile( c_Minecraft, { .Mode = EGlyphRenderMode::Pixel } );
    REQUIRE( face );

    auto& outline = static_cast<FreeType::FontFace&>( *face );
    REQUIRE( outline.GetPixelGridSource() == FreeType::EPixelGridSource::OutlineGrid );

    // Minecraft.ttf: 1000 units per em, 70-unit pixels, glyphs offset 50 units from the grid.
    REQUIRE( face->Metrics().NativePixelSize == Catch::Approx( 1000.f / 70.f ).epsilon( 0.001 ) );

    SECTION( "Glyphs are exactly 1-bit and on whole pixels" )
    {
        const GlyphBitmap h = Rasterize( *face, U'H', EGlyphRenderMode::Pixel );
        REQUIRE( h.Width == 7 );
        REQUIRE( h.Height == 10 );
        REQUIRE( h.Bearing[1] == Catch::Approx( 10.f ) );
        for ( u32 i = 0; i < h.Width * h.Height; ++i )
            REQUIRE( ( h.Pixels[i][3] == 0 || h.Pixels[i][3] == 255 ) );
    }

    SECTION( "An outline font is not mistaken for a pixel font" )
    {
        Unique<IFontFace> roboto = loader.LoadFromFile( c_Roboto, { .Mode = EGlyphRenderMode::Pixel } );
        REQUIRE( static_cast<FreeType::FontFace&>( *roboto ).GetPixelGridSource() == FreeType::EPixelGridSource::Fallback );
    }

    SECTION( "A manual native size overrides detection" )
    {
        Unique<IFontFace> manual = loader.LoadFromFile( c_Minecraft, { .Mode = EGlyphRenderMode::Pixel, .NativePixelSize = 20.f } );
        REQUIRE( manual->Metrics().NativePixelSize == Catch::Approx( 20.f ) );
        REQUIRE( static_cast<FreeType::FontFace&>( *manual ).GetPixelGridSource() == FreeType::EPixelGridSource::Manual );
    }
}

TEST_CASE( "Rich text over FreeType faces", "[Font][FreeType][RichText]" )
{
    FreeType::FontLoader loader;
    FontLibrary fonts;
    const FontFamilyHandle roboto = fonts.RegisterFamily( "Roboto"_id );
    fonts.AddFaceToFamily( roboto, loader.LoadFromFile( c_Roboto ), { EFontWeight::Medium } );
    const FontFamilyHandle pixel = fonts.RegisterFamily( "Minecraft"_id );
    fonts.AddFaceToFamily( pixel, loader.LoadFromFile( c_Minecraft, { .Mode = EGlyphRenderMode::Pixel } ) );

    TextMetrics metrics{ fonts };
    const TextLayoutStyle style{ .Family = roboto, .Size = 16_u };

    const TextSpan spans[] = {
        { 6, 11,  { .Family = pixel } },
        { 11, 17, { .Weight = EFontWeight::Bold } },
    };

    const auto prepared = metrics.Prepare( StyledTextView{ "Hello pixel world", spans }, style );
    REQUIRE( prepared );
    REQUIRE( Size( prepared->Runs ) == 3 );
    REQUIRE( prepared->Runs[2].Face.SynthBold );

    const auto shaped = metrics.Shape( *prepared, style );
    REQUIRE( shaped );
    REQUIRE( shaped->LineCount() == 1 );
    REQUIRE( Size( shaped->Runs ) == 3 );
    REQUIRE( shaped->Runs[1].Mode == EGlyphRenderMode::Pixel );
    REQUIRE( shaped->MaxWidth > 0_u );
}
#endif
