#pragma once
#include "../Core.h"
#include "IFontFace.h"
#include "Unicode.h"

#include <cmath>

// Face-agnostic shaping helpers shared by TextMetrics and fast text.
namespace RatUI::TextShaping
{
    inline constexpr f32 c_SynthItalicSlant = 0.2126f; ///< ~12 degrees, same as FreeType's FT_GlyphSlot_Oblique.

    /** @brief Extra stroke width for synthetic bold in em: 1/24 at small sizes down to 1/32 at large ones (like Skia). */
    inline f32 SynthBoldEm( Unit a_Size )
    {
        constexpr f32 c_SmallSize = 9.f, c_LargeSize = 36.f;
        const f32 t = std::clamp( ( a_Size.ToFloat() - c_SmallSize ) / ( c_LargeSize - c_SmallSize ), 0.f, 1.f );
        return ( 1.f / 24.f ) + t * ( ( 1.f / 32.f ) - ( 1.f / 24.f ) );
    }

    inline bool IsPixelFace( const IFontFace& a_Face )
    {
        return a_Face.GetMode() == EGlyphRenderMode::Pixel && a_Face.Metrics().NativePixelSize > 0.f;
    }

    /** @brief Pixel faces snap to a whole multiple (>= 1) of their native size. */
    inline Unit EffectiveSize( const IFontFace& a_Face, Unit a_Size )
    {
        if ( !IsPixelFace( a_Face ) )
            return a_Size;

        const f32 native = a_Face.Metrics().NativePixelSize;
        const f32 scale  = std::max( 1.f, std::round( a_Size.ToFloat() / native ) );
        return Unit{ native * scale };
    }

    /** @brief One native pixel in layout units, or 0 for scalable faces. */
    inline Unit NativePixelUnit( const IFontFace& a_Face, Unit a_EffectiveSize )
    {
        return IsPixelFace( a_Face ) ? a_EffectiveSize / a_Face.Metrics().NativePixelSize : 0_u;
    }

    inline Unit SnapToGrid( Unit a_Value, Unit a_Grid )
    {
        return a_Grid > 0_u ? Unit{ std::round( a_Value.ToFloat() / a_Grid.ToFloat() ) * a_Grid.ToFloat() } : a_Value;
    }

    /** @brief */
    inline Unit EmToUnit( const IFontFace& a_Face, FontUnit a_Em, Unit a_EffectiveSize )
    {
        return SnapToGrid( ToUnit( a_Em, a_EffectiveSize ), NativePixelUnit( a_Face, a_EffectiveSize ) );
    }

    struct PieceParams
    {
        ResolvedFace   Face{};
        Unit           Size{ 16_u }; ///< Effective size.
        Unit           LetterSpacing{ 0_u };
        Unit           WordSpacing{ 0_u };
        ETextDirection Direction{ ETextDirection::Auto };
        EScript        Script{ EScript::Invalid };
        bool           SimpleShaping{ false }; ///< cmap shaping only (fast text).
    };

    /** @brief */
    inline Unit ShapePiece( IFontFace& a_Face, StringView a_Text, u32 a_ClusterBase, const PieceParams& a_Params, Array<ShapedGlyph>& o_Glyphs )
    {
        const size first = Size( o_Glyphs );
        const ShapeRunParams runParams{
            .Size        = a_Params.Size,
            .Direction   = a_Params.Direction,
            .Script      = a_Params.Script,
            .ClusterBase = a_ClusterBase,
        };

        if ( a_Params.SimpleShaping )
            SimpleShapeRun( a_Face, a_Text, runParams, o_Glyphs );
        else
            a_Face.ShapeRun( a_Text, runParams, o_Glyphs );

        // Pixel bold adds one native pixel, outline bold adds the stroke growth.
        const Unit pixel     = NativePixelUnit( a_Face, a_Params.Size );
        const Unit boldExtra = !a_Params.Face.SynthBold ? 0_u
                             : pixel > 0_u ? pixel
                                           : a_Params.Size * SynthBoldEm( a_Params.Size );

        Unit width = 0_u;
        const size count = Size( o_Glyphs );
        for ( size i = first; i < count; ++i )
        {
            ShapedGlyph& glyph = o_Glyphs[i];

            if ( pixel > 0_u )
            {
                glyph.XAdvance = SnapToGrid( glyph.XAdvance, pixel );
                glyph.XOffset  = SnapToGrid( glyph.XOffset, pixel );
                glyph.YOffset  = SnapToGrid( glyph.YOffset, pixel );
            }

            const bool clusterEnd = ( i + 1 == count ) || o_Glyphs[i + 1].Cluster != glyph.Cluster;
            if ( clusterEnd )
            {
                if ( glyph.XAdvance > 0_u )
                    glyph.XAdvance += boldExtra;

                glyph.XAdvance += a_Params.LetterSpacing;

                if ( a_Params.WordSpacing != 0_u && glyph.Cluster >= a_ClusterBase
                     && Unicode::IsWhitespaceCluster( a_Text, glyph.Cluster - a_ClusterBase ) )
                    glyph.XAdvance += a_Params.WordSpacing;
            }

            width += glyph.XAdvance;
        }

        return width;
    }

} // namespace RatUI::TextShaping
