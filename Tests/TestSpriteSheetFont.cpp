#include "TestFontCommon.h"
#include <RatUI/Text/PixelGlyphOps.h>

namespace
{
    /** @brief Builds a 3-cell (6x8) sheet: 'A' = solid 3x6 block, 'I' = 1x6 column, ' ' = blank. Baseline row 7. */
    struct SheetFixture
    {
        static constexpr u32 c_CellW = 6, c_CellH = 8, c_Cells = 3;
        Array<Color> Pixels = Array<Color>( c_CellW * c_Cells * c_CellH, Colors::Transparent );

        SheetFixture()
        {
            const auto set = [&]( u32 a_Cell, u32 a_X, u32 a_Y, Color a_Color )
            {
                Pixels[a_Y * c_CellW * c_Cells + a_Cell * c_CellW + a_X] = a_Color;
            };

            for ( u32 y = 1; y <= 6; ++y )
            {
                for ( u32 x = 1; x <= 3; ++x )
                    set( 0, x, y, Colors::Red ); // 'A'
                set( 1, 2, y, Colors::Red );     // 'I'
            }
        }

        SpriteSheetFontDesc Desc( bool a_Monospace = false ) const
        {
            SpriteSheetFontDesc desc;
            desc.Pixels     = Data( Pixels );
            desc.ImageSize  = { c_CellW * c_Cells, c_CellH };
            desc.CellSize   = { c_CellW, c_CellH };
            desc.Characters = "AI ";
            desc.Baseline   = 7;
            desc.Monospace  = a_Monospace;
            desc.Kerning    = { { U'A', U'I', -1 } };
            return desc;
        }
    };

    f32 NativeAdvance( const IFontFace& a_Face, codepoint a_Codepoint )
    {
        return a_Face.GetAdvance( a_Face.GetGlyphIndex( a_Codepoint ) ).ToFloat() * a_Face.Metrics().NativePixelSize;
    }
}

TEST_CASE( "Sprite sheet faces trim proportional glyphs and expose pixel metrics", "[Font][SpriteSheet]" )
{
    SheetFixture sheet;
    Unique<IFontFace> face = MakeSpriteSheetFace( sheet.Desc() );
    REQUIRE( face );

    const FontFaceMetrics& metrics = face->Metrics();
    REQUIRE( metrics.Mode == EGlyphRenderMode::Pixel );
    REQUIRE( metrics.NativePixelSize == Catch::Approx( 8.f ) );
    REQUIRE( metrics.Ascender.ToFloat() == Catch::Approx( 7.f / 8.f ) );
    REQUIRE( metrics.Descender.ToFloat() == Catch::Approx( -1.f / 8.f ) );

    SECTION( "Advances are inked width + letter spacing" )
    {
        REQUIRE( NativeAdvance( *face, U'A' ) == Catch::Approx( 4.f ) );
        REQUIRE( NativeAdvance( *face, U'I' ) == Catch::Approx( 2.f ) );
        REQUIRE( NativeAdvance( *face, U' ' ) == Catch::Approx( 3.f ) ); // Blank cell: half a cell.
    }

    SECTION( "Kerning pairs are in native pixels" )
    {
        const FontUnit kerning = face->GetKerning( face->GetGlyphIndex( U'A' ), face->GetGlyphIndex( U'I' ) );
        REQUIRE( kerning.ToFloat() * 8.f == Catch::Approx( -1.f ) );
    }

    SECTION( "Rasterized glyphs are trimmed, masked to white, and positioned on the baseline" )
    {
        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = face->GetGlyphIndex( U'A' ), .Mode = EGlyphRenderMode::Pixel }, bitmap ) );
        REQUIRE( bitmap.Width == 3 );
        REQUIRE( bitmap.Height == 6 );
        REQUIRE( bitmap.Bearing[0] == Catch::Approx( 0.f ) );
        REQUIRE( bitmap.Bearing[1] == Catch::Approx( 6.f ) );
        REQUIRE( bitmap.Pixels[0] == Color{ 255, 255, 255, 255 } );
    }

    SECTION( "Missing characters map to a visible .notdef box" )
    {
        REQUIRE( ToUnderlying( face->GetGlyphIndex( U'Z' ) ) == 0 );

        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = GlyphID{ 0 }, .Mode = EGlyphRenderMode::Pixel }, bitmap ) );
        REQUIRE( bitmap.Width > 0 );
        REQUIRE( bitmap.Height > 0 );
    }

    SECTION( "Synthetic bold widens by one pixel" )
    {
        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = face->GetGlyphIndex( U'A' ), .Mode = EGlyphRenderMode::Pixel, .SynthBold = true }, bitmap ) );
        REQUIRE( bitmap.Width == 4 );
    }
}

TEST_CASE( "Sprite sheet faces support monospace and colour", "[Font][SpriteSheet]" )
{
    SheetFixture sheet;

    SECTION( "Monospace advances by the cell and keeps the in-cell position" )
    {
        SpriteSheetFontDesc desc = sheet.Desc( true );
        Unique<IFontFace> face = MakeSpriteSheetFace( desc );
        REQUIRE( NativeAdvance( *face, U'I' ) == Catch::Approx( 6.f ) );

        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = face->GetGlyphIndex( U'I' ), .Mode = EGlyphRenderMode::Pixel }, bitmap ) );
        REQUIRE( bitmap.Bearing[0] == Catch::Approx( 2.f ) );
    }

    SECTION( "PreserveColor keeps the sheet's colours" )
    {
        SpriteSheetFontDesc desc = sheet.Desc();
        desc.PreserveColor = true;
        Unique<IFontFace> face = MakeSpriteSheetFace( desc );

        GlyphBitmap bitmap;
        REQUIRE( face->RasterizeGlyph( { .Glyph = face->GetGlyphIndex( U'A' ), .Mode = EGlyphRenderMode::Pixel }, bitmap ) );
        REQUIRE( bitmap.Pixels[0] == Colors::Red );
    }

    SECTION( "Invalid descriptions are rejected" )
    {
        SpriteSheetFontDesc desc = sheet.Desc();
        desc.CellSize = { 0, 0 };
        REQUIRE_FALSE( MakeSpriteSheetFace( desc ) );
    }
}

TEST_CASE( "Pixel glyph ops keep strokes on the grid", "[Font][PixelGlyphOps]" )
{
    // Row pattern "#.#" (1px counter between two stems), 1 row high.
    const Color on = Colors::White, off = Colors::Transparent;

    SECTION( "Embolden grows strokes but keeps 1px counters open" )
    {
        Array<Color> pixels{ on, off, on };
        u32 width = 3;
        PixelGlyphOps::Embolden( pixels, width, 1 );

        REQUIRE( width == 4 );
        REQUIRE( pixels[0][3] == 255 );
        REQUIRE( pixels[1][3] == 0 );   // Counter preserved.
        REQUIRE( pixels[2][3] == 255 );
        REQUIRE( pixels[3][3] == 255 ); // Last stem grew.
    }

    SECTION( "Embolden fills wider gaps by one pixel" )
    {
        Array<Color> pixels{ on, off, off, on };
        u32 width = 4;
        PixelGlyphOps::Embolden( pixels, width, 1 );
        REQUIRE( pixels[1][3] == 255 );
        REQUIRE( pixels[2][3] == 0 );
    }

    SECTION( "Italicize shifts rows above the baseline to the right" )
    {
        // 8 rows, 1px wide column, baseline below the last row.
        Array<Color> pixels( 8, on );
        u32 width = 1;
        const u32 grown = PixelGlyphOps::Italicize( pixels, width, 8, 8 );

        REQUIRE( grown == 1 );
        REQUIRE( width == 2 );
        REQUIRE( pixels[0 * width + 1][3] == 255 ); // Top row shifted.
        REQUIRE( pixels[7 * width + 0][3] == 255 ); // Bottom row in place.
    }
}
