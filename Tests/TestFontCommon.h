#pragma once

/** @file TestFontCommon.h @brief In-memory IFontFace for font / text tests. */

#include "TestCommon.h"
#include <string>

/** @brief Fixed codepoint set, one advance, solid-rectangle glyphs and round metrics for exact checks. */
class FixedFace final : public IFontFace
{
public:
    struct Desc
    {
        std::u32string   Characters{ U" abcdefghijklmnopqrstuvwxyz-." };
        f32              AdvanceEm{ 0.5f };
        EGlyphRenderMode Mode{ EGlyphRenderMode::MTSDF };
        f32              NativePixelSize{ 0.f };
        FontUnit         Ascender{ 0.8_fu };
        FontUnit         Descender{ -0.2_fu };
        FontUnit         LineGap{ 0_fu };
        u32              BitmapWidth{ 10 };
        u32              BitmapHeight{ 12 };
        String           Name{ "Fixed" };
    };

    explicit FixedFace( Desc a_Desc ) : m_Desc( std::move( a_Desc ) )
    {
        m_Metrics.Ascender        = m_Desc.Ascender;
        m_Metrics.Descender       = m_Desc.Descender;
        m_Metrics.LineGap         = m_Desc.LineGap;
        m_Metrics.Mode            = m_Desc.Mode;
        m_Metrics.NativePixelSize = m_Desc.NativePixelSize;
    }

    EFontFaceKind Kind() const override { return EFontFaceKind::Custom; }
    const FontFaceMetrics& Metrics() const override { return m_Metrics; }
    StringView GetDebugName() const override { return m_Desc.Name; }

    GlyphID GetGlyphIndex( codepoint a_Codepoint ) const override
    {
        const size index = m_Desc.Characters.find( a_Codepoint );
        return index == std::u32string::npos ? GlyphID{ 0 } : GlyphID{ static_cast<u32>( index + 1 ) };
    }

    FontUnit GetAdvance( GlyphID ) const override { return FontUnit{ m_Desc.AdvanceEm }; }

    bool RasterizeGlyph( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap ) override
    {
        ++RasterizeCount;
        o_Bitmap = {};

        const u32 index = ToUnderlying( a_Request.Glyph );
        if ( index != 0 && m_Desc.Characters[index - 1] == U' ' )
            return true; // Blank.

        Resize( m_Pixels, static_cast<size>( m_Desc.BitmapWidth ) * m_Desc.BitmapHeight );
        std::fill( Begin( m_Pixels ), End( m_Pixels ), Colors::White );

        o_Bitmap.Pixels  = Data( m_Pixels );
        o_Bitmap.Width   = m_Desc.BitmapWidth;
        o_Bitmap.Height  = m_Desc.BitmapHeight;
        o_Bitmap.Bearing = { 0.f, static_cast<f32>( m_Desc.BitmapHeight ) };
        return true;
    }

    u32 RasterizeCount{ 0 };

private:
    Desc            m_Desc;
    FontFaceMetrics m_Metrics;
    Array<Color>    m_Pixels;
};

/** @brief Adds a FixedFace to a family and returns the face handle. */
inline FontFaceHandle AddFixedFace( FontLibrary& a_Fonts, FontFamilyHandle a_Family, FixedFace::Desc a_Desc, FontFaceDesc a_FaceDesc = {} )
{
    return a_Fonts.AddFaceToFamily( a_Family, MakeUnique<FixedFace>( std::move( a_Desc ) ), a_FaceDesc );
}
