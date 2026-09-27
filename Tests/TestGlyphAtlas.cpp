#include "TestFontCommon.h"

TEST_CASE( "SkylinePacker packs without overlap", "[Text][GlyphAtlas]" )
{
    SkylinePacker packer;
    packer.Reset( 64, 64 );

    Array<Rectu16> placed;
    const u16 sizes[][2] = { { 10, 20 }, { 30, 8 }, { 12, 12 }, { 20, 5 }, { 7, 30 }, { 16, 16 }, { 25, 10 } };
    for ( const auto& size : sizes )
    {
        const auto position = packer.Allocate( size[0], size[1] );
        REQUIRE( position );
        const Rectu16 rect{ *position, { size[0], size[1] } };
        REQUIRE( rect.Right() <= 64 );
        REQUIRE( rect.Bottom() <= 64 );

        for ( const Rectu16& other : placed )
        {
            const bool overlap = rect.Left() < other.Right() && other.Left() < rect.Right()
                              && rect.Top() < other.Bottom() && other.Top() < rect.Bottom();
            REQUIRE_FALSE( overlap );
        }
        PushBack( placed, rect );
    }

    REQUIRE_FALSE( packer.Allocate( 65, 1 ) );
}

TEST_CASE( "GlyphAtlas caches glyphs per render mode, size and synthesis", "[Text][GlyphAtlas]" )
{
    Null::NullRenderer renderer;
    FontLibrary fonts;
    const FontFamilyHandle family = fonts.RegisterFamily( "F"_id );
    const FontFaceHandle faceHandle = AddFixedFace( fonts, family, { .BitmapWidth = 8, .BitmapHeight = 8 } );
    FixedFace& face = static_cast<FixedFace&>( *fonts.GetFace( faceHandle ) );

    GlyphAtlas atlas( renderer, fonts, GlyphAtlasConfig{ .PageSize = 128, .MaxPagesPerMode = 2 } );
    const ResolvedFace regular{ .Face = faceHandle };
    const GlyphID glyph = face.GetGlyphIndex( U'a' );

    const auto first = atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::Raster, 16 ) );
    REQUIRE( first );
    REQUIRE_FALSE( first->IsBlank() );
    REQUIRE( face.RasterizeCount == 1 );

    SECTION( "A second lookup is served from the cache" )
    {
        const auto second = atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::Raster, 16 ) );
        REQUIRE( second->Rect == first->Rect );
        REQUIRE( face.RasterizeCount == 1 );
    }

    SECTION( "Raster sizes and synthesized styles are separate entries" )
    {
        atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::Raster, 17 ) );
        atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( ResolvedFace{ .Face = faceHandle, .SynthBold = true }, glyph, EGlyphRenderMode::Raster, 16 ) );
        REQUIRE( face.RasterizeCount == 3 );
        REQUIRE( atlas.GetGlyphCount() == 3 );
    }

    SECTION( "MTSDF and Pixel entries are size independent" )
    {
        REQUIRE( GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::MTSDF, 12 ) == GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::MTSDF, 40 ) );
        REQUIRE( GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::Pixel, 12 ) == GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::Pixel, 40 ) );

        // MTSDF synthesizes in the shader, so faux bold shares the regular bitmap.
        REQUIRE( GlyphCacheKey::For( ResolvedFace{ .Face = faceHandle, .SynthBold = true }, glyph, EGlyphRenderMode::MTSDF, 0 )
              == GlyphCacheKey::For( regular, glyph, EGlyphRenderMode::MTSDF, 0 ) );
    }

    SECTION( "Blank glyphs are cached without using page space" )
    {
        const auto space = atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( regular, face.GetGlyphIndex( U' ' ), EGlyphRenderMode::Raster, 16 ) );
        REQUIRE( space );
        REQUIRE( space->IsBlank() );
    }

    SECTION( "Every mode has a white block for decorations" )
    {
        REQUIRE( atlas.GetWhiteBlock( EGlyphRenderMode::Pixel ) );
        REQUIRE( atlas.GetPageCount( EGlyphRenderMode::Pixel ) == 1 );
    }
}

TEST_CASE( "GlyphAtlas grows and recycles pages instead of failing", "[Text][GlyphAtlas]" )
{
    Null::NullRenderer renderer;
    FontLibrary fonts;
    const FontFamilyHandle family = fonts.RegisterFamily( "F"_id );
    // 30x30 glyphs in 64x64 pages: one glyph per page.
    const FontFaceHandle faceHandle = AddFixedFace( fonts, family, { .BitmapWidth = 30, .BitmapHeight = 30 } );
    const IFontFace& face = *fonts.GetFace( faceHandle );

    GlyphAtlas atlas( renderer, fonts, GlyphAtlasConfig{ .PageSize = 64, .MaxPagesPerMode = 2 } );
    const ResolvedFace resolved{ .Face = faceHandle };
    const auto key = [&]( codepoint a_Codepoint ) { return GlyphCacheKey::For( resolved, face.GetGlyphIndex( a_Codepoint ), EGlyphRenderMode::Raster, 30 ); };

    // Fill both pages within one frame.
    u32 placed = 0;
    for ( codepoint cp = U'a'; cp <= U'z' && atlas.GetPageCount( EGlyphRenderMode::Raster ) <= 2; ++cp )
    {
        if ( !atlas.GetOrRasterizeGlyph( key( cp ) ) )
            break;
        ++placed;
    }

    REQUIRE( atlas.GetPageCount( EGlyphRenderMode::Raster ) == 2 );
    REQUIRE( placed >= 2 );

    SECTION( "Pages in use this frame are never recycled" )
    {
        REQUIRE_FALSE( atlas.GetOrRasterizeGlyph( key( U'z' ) ) );
        REQUIRE( atlas.GetEvictionCount() == 0 );
    }

    SECTION( "The least recently used page is recycled on a later frame" )
    {
        atlas.BeginFrame();
        const u32 before = atlas.GetGlyphCount();
        REQUIRE( atlas.GetOrRasterizeGlyph( key( U'z' ) ) );
        REQUIRE( atlas.GetEvictionCount() == 1 );
        REQUIRE( atlas.GetGlyphCount() < before + 1 ); // The recycled page's glyphs were dropped.
        REQUIRE( atlas.GetPageCount( EGlyphRenderMode::Raster ) == 2 );
    }
}
