#pragma once
#include "../../RatUI.h"
#include "Config.h"
#include "FontCache.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cctype>

namespace RatUI::FreeType::TextUtil
{
    using ::RatUI::Unicode::UTF8Iterator;
    using ::RatUI::Unicode::UTF8Range;

    /**
     * @brief Measures the pixel width of a single (newline-free) line of text.
     * 
	 * TODO: This shapes the line every time its called. May need to add a separate MeasureLineWidth that takes already-shaped glyphs if this becomes a bottleneck?
     *
     * @param a_Font  The loaded Font (provides FTFace + advance cache).
     * @param a_Line  A newline-free string view to measure.
     * @param a_Style TextLayoutStyle for letter spacing.
     * @return The measured width in pixels.
     */
    inline Unit MeasureLineWidth(
        Font& a_Font,
        StringView a_Line,
        const TextLayoutStyle& a_Style )
    {
        if ( Empty( a_Line ) )
            return 0_u;

        hb_buffer_t* buffer = a_Font.GetHBBuffer();

        hb_buffer_clear_contents( buffer );
        hb_buffer_add_utf8( buffer,
                           Data( a_Line ),
                           (int)Size( a_Line ),
                           0,
                           (int)Size( a_Line ) );

        hb_buffer_guess_segment_properties( buffer );

        hb_shape( a_Font.GetHBFont(), buffer, nullptr, 0 );

        u32 glyphCount = 0;
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions( buffer, &glyphCount );

        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos( buffer, &glyphCount );

        if ( !positions || !infos || glyphCount == 0 )
            return 0_u;

		const Unit fontSize = a_Style.Size;

        // Same EM normalization as ShapeLine: HarfBuzz gives 26.6 fixed-point scaled-pixel
        // values; divide by 64 (26.6 factor) then by ppem (pixels per EM) to get FontUnit.
        // The fallback 4096.f = 64*64 matches the default ppem set in Font::LoadFromFace.
        const f32 ppem      = static_cast<f32>( a_Font.GetFace()->size->metrics.y_ppem );
        const f32 emNormDiv = ( ppem > 0.f ) ? ( 64.f * ppem ) : 4096.f;

        Unit width = 0_u;

        u32 prevCluster       = infos[0].cluster;
        u32 clusterCount      = 1;
        u32 spaceClusterCount = 0;

		width += ToUnit( FontUnit{ static_cast<f32>( positions[0].x_advance ) / emNormDiv }, fontSize );

        if ( a_Style.WordSpacing != 0_u && Unicode::IsWhitespaceCluster( a_Line, infos[0].cluster ) )
            ++spaceClusterCount;

        for ( u32 i = 1; i < glyphCount; ++i )
        {
            width += ToUnit( FontUnit{ static_cast<f32>( positions[i].x_advance ) / emNormDiv }, fontSize );

            if ( infos[i].cluster != prevCluster )
            {
                ++clusterCount;
                prevCluster = infos[i].cluster;

                if ( a_Style.WordSpacing != 0_u && Unicode::IsWhitespaceCluster( a_Line, infos[i].cluster ) )
                    ++spaceClusterCount;
            }
        }

        if ( a_Style.LetterSpacing != 0_u && clusterCount > 1 )
            width += a_Style.LetterSpacing * ( clusterCount - 1 );

        if ( a_Style.WordSpacing != 0_u && spaceClusterCount > 0 )
            width += a_Style.WordSpacing * static_cast<f32>( spaceClusterCount );

        return width;
    }

    /**
     * @brief Applies the specified text transformation (uppercase, lowercase, capitalize) to a UTF-8 encoded string.

      * TODO: Support locale-aware transformations and proper word-boundary detection for capitalization.
      *       Probably need to use ICU. I miss ascii.
     */
    inline String ApplyTextTransform( String&& a_Text, ETextTransform a_Transform )
    {
    #if RATUI_FREETYPE_WITH_ICU
	#error "Text transformations are not yet implemented. ICU is included in the build, but ApplyTextTransform needs to be implemented to use it."
    #else
		return Unicode::ApplyTextTransformASCII( std::move( a_Text ), a_Transform );
    #endif
    }

    /**
     * @brief Truncates a shaped line in-place and appends shaped ellipsis glyphs.
     *
     * @param io_Text        The shaped text to modify.
     * @param a_LineIndex    Line index to truncate.
     * @param a_MaxWidth     Maximum allowed width.
     * @param a_Ellipsis     Pre-shaped ellipsis (same font + size).
     *
     * @return True if truncation occurred.
     */
    inline bool TruncateShapedLineWithEllipsis(
        ShapedText&       o_Text,
        u32               a_LineIndex,
        Unit              a_MaxWidth,
        const ShapedText& a_Ellipsis )
    {
        if (a_LineIndex >= o_Text.LineCount())
            return false;

        ShapedLine& line = o_Text.Lines[a_LineIndex];

        // Already fits
        if (line.Width <= a_MaxWidth)
            return false;

        if (Empty(a_Ellipsis.Glyphs) || Empty(a_Ellipsis.Lines))
            return false;

        const Unit ellipsisWidth = a_Ellipsis.Lines[0].Width;

        // Even "..." cannot fit
        if (ellipsisWidth > a_MaxWidth)
        {
            line.End   = line.Start;
            line.Width = 0_u;
            return true;
        }

        const Unit prefixBudget = a_MaxWidth - ellipsisWidth;

        Unit accumulated = 0_u;
        u32  newEnd      = line.Start;

        for (u32 i = line.Start; i < line.End; ++i)
        {
            const ShapedGlyph& g = o_Text.Glyphs[i];

            const Unit adv = ToUnit(g.XAdvance, o_Text.FontSize);

            if (accumulated + adv > prefixBudget)
                break;

            accumulated += adv;
            ++newEnd;
        }

        // Adjust line range
        line.End   = newEnd;
        line.Width = accumulated + ellipsisWidth;

        // Append ellipsis glyphs to global glyph buffer
        const u32 insertPos = line.End;
        const u32 ellipsisCount = static_cast<u32>(Size(a_Ellipsis.Glyphs));

        Insert(
            o_Text.Glyphs,
            Begin( o_Text.Glyphs ) + insertPos,
            Begin(a_Ellipsis.Glyphs),
            End(a_Ellipsis.Glyphs)
        );

        line.End += ellipsisCount;

        // Shift subsequent lines forward
        for (u32 i = a_LineIndex + 1; i < o_Text.LineCount(); ++i)
        {
            o_Text.Lines[i].Start += ellipsisCount;
            o_Text.Lines[i].End   += ellipsisCount;
        }

        return true;
    }

} // namespace RatUI::FreeType::TextUtil
