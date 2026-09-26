#pragma once
#include "../Core.h"
#include "Font.h"
#include "Text.h"
#include "Unicode.h"

namespace RatUI
{
    /** @brief Lets backends reach their own face data without RTTI. */
    enum class EFontFaceKind : u8
    {
        Custom,
        SpriteSheet,
        FreeType,
    };

    struct ShapeRunParams
    {
        Unit           Size       { 16_u };
        ETextDirection Direction  { ETextDirection::Auto };
        EScript        Script     { EScript::Invalid };
        u32            ClusterBase{ 0 }; ///< Added to every glyph's Cluster (the run's byte offset in the full text).
    };

    struct GlyphRasterRequest
    {
        GlyphID          Glyph{};
        EGlyphRenderMode Mode{ EGlyphRenderMode::MTSDF };
        u16              PixelSize{ 0 };        ///< MTSDF: SDF base size. Raster: ppem. Pixel: unused.
        f32              SDFPixelRange{ 16.f };
        bool             SynthBold  : 1 = false; ///< Raster / Pixel only, MTSDF bolds in the shader.
        bool             SynthItalic: 1 = false; ///< Raster / Pixel only, MTSDF shears the quad.
    };

    /** @brief RGBA8 glyph bitmap, Y-down. Owned by the face and valid until its next RasterizeGlyph() call. */
    struct GlyphBitmap
    {
        const Color* Pixels{ nullptr };
        u32          Width { 0 };
        u32          Height{ 0 };
        Vec2f        Bearing{ 0.f, 0.f }; ///< Pen to top-left, Y-up. MTSDF: em. Raster / Pixel: pixels.
    };

    /** @brief A font face, whatever its source (outline font, pixel TTF, sprite sheet...). */
    class IFontFace
    {
    public:
        virtual ~IFontFace() = default;

        virtual EFontFaceKind Kind() const = 0;
        virtual const FontFaceMetrics& Metrics() const = 0;

        /** @brief Returns GlyphID{ 0 } (.notdef) if the face doesn't have the codepoint. */
        virtual GlyphID GetGlyphIndex( codepoint a_Codepoint ) const = 0;
        virtual FontUnit GetAdvance( GlyphID a_Glyph ) const = 0;
        virtual FontUnit GetKerning( GlyphID /*a_Left*/, GlyphID /*a_Right*/ ) const { return 0_fu; }

        /** @brief An empty bitmap with true is valid (e.g. space). */
        virtual bool RasterizeGlyph( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap ) = 0;

        /** @brief */
        virtual void ShapeRun( StringView a_Text, const ShapeRunParams& a_Params, Array<ShapedGlyph>& o_Glyphs );

        virtual StringView GetDebugName() const { return {}; }

        bool HasGlyph( codepoint a_Codepoint ) const { return ToUnderlying( GetGlyphIndex( a_Codepoint ) ) != 0; }
        EGlyphRenderMode GetMode() const { return Metrics().Mode; }
    };

    /** @brief */
    inline void SimpleShapeRun( const IFontFace& a_Face, StringView a_Text, const ShapeRunParams& a_Params, Array<ShapedGlyph>& o_Glyphs )
    {
        const size first = Size( o_Glyphs );
        GlyphID prev{ 0 };

        for ( Unicode::UTF8Iterator it( a_Text ); it; ++it )
        {
            const GlyphID glyph = a_Face.GetGlyphIndex( *it );

            if ( Size( o_Glyphs ) > first && ToUnderlying( prev ) != 0 && ToUnderlying( glyph ) != 0 )
                Back( o_Glyphs ).XAdvance += ToUnit( a_Face.GetKerning( prev, glyph ), a_Params.Size );

            EmplaceBack( o_Glyphs, ShapedGlyph{
                .GlyphIndex = glyph,
                .Cluster    = a_Params.ClusterBase + static_cast<u32>( it.ByteIndex() ),
                .XAdvance   = ToUnit( a_Face.GetAdvance( glyph ), a_Params.Size ),
            } );

            prev = glyph;
        }
    }

    inline void IFontFace::ShapeRun( StringView a_Text, const ShapeRunParams& a_Params, Array<ShapedGlyph>& o_Glyphs )
    {
        SimpleShapeRun( *this, a_Text, a_Params, o_Glyphs );
    }

} // namespace RatUI
