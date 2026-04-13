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
    inline f32 MeasureLineWidth(
        Font& a_Font,
        StringView a_Line,
        const TextLayoutStyle& a_Style )
    {
        if ( Empty( a_Line ) )
            return 0.f;

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
            return 0.f;

        f32 width = 0.f;

        u32 prevCluster       = infos[0].cluster;
        u32 clusterCount      = 1;
        u32 spaceClusterCount = 0;

        width += positions[0].x_advance / 64.f;

        if ( a_Style.WordSpacing != 0.f && Unicode::IsWhitespaceCluster( a_Line, infos[0].cluster ) )
            ++spaceClusterCount;

        for ( u32 i = 1; i < glyphCount; ++i )
        {
            width += positions[i].x_advance / 64.f;

            if ( infos[i].cluster != prevCluster )
            {
                ++clusterCount;
                prevCluster = infos[i].cluster;

                if ( a_Style.WordSpacing != 0.f && Unicode::IsWhitespaceCluster( a_Line, infos[i].cluster ) )
                    ++spaceClusterCount;
            }
        }

        if ( a_Style.LetterSpacing != 0.f && clusterCount > 1 )
            width += a_Style.LetterSpacing * ( clusterCount - 1 );

        if ( a_Style.WordSpacing != 0.f && spaceClusterCount > 0 )
            width += a_Style.WordSpacing * static_cast<f32>( spaceClusterCount );

        return width;
    }

    /**
     * @brief Applies the specified text transformation (uppercase, lowercase, capitalize) to a UTF-8 encoded string.

      * TODO: Support locale-aware transformations and proper word-boundary detection for capitalization.
      *       Probably need to use ICU. I miss ascii.
     */
    inline String ApplyTextTransform( StringView a_Text, ETextTransform a_Transform )
    {
    #if RATUI_FREETYPE_WITH_ICU
	#error "Text transformations are not yet implemented. ICU is included in the build, but ApplyTextTransform needs to be implemented to use it."
    #else
		String result;
		Unicode::ApplyTextTransformASCII( a_Text, result, a_Transform );
		return result;
    #endif
    }

    /**
     * @brief Truncates a line so that (prefix + "...") fits within a_MaxWidth.
     * @param a_Font           Font providing FTFace and advance cache.
     * @param a_Line           The line to truncate (must be newline-free).
     * @param a_Style          Text style (letter spacing).
     * @param a_MaxWidth       Maximum allowed width of the final string.
     * @param a_ForceEllipsis  When true, always appends "..." even if the line fits.
     * @return The truncated string, or an empty string if even "..." does not fit.
     */
    inline String TruncateLineWithEllipsis(
        Font&                  a_Font,
        StringView             a_Line,
        const TextLayoutStyle& a_Style,
        f32                    a_MaxWidth,
        bool                   a_ForceEllipsis )
    {
        FT_Face face = a_Font.GetFace();

        if ( !a_ForceEllipsis && MeasureLineWidth( a_Font, a_Line, a_Style ) <= a_MaxWidth )
            return String( Begin( a_Line ), End( a_Line ) );

        constexpr StringView c_Ellipsis = "...";
        const f32 ellipsisWidth = MeasureLineWidth( a_Font, c_Ellipsis, a_Style );
        if ( ellipsisWidth > a_MaxWidth )
            return {};

        const u32 dotGlyphIdx = FT_Get_Char_Index( face, U'.' );

        size_t bestPrefixByteCount = 0;
        f32    prefixWidth         = 0.f;
        u32    prevGlyphIdx        = 0;
        bool   hasPrev             = false;

        UTF8Range range{ a_Line };
		for ( auto it = range.begin(); it != range.end(); ++it )
        {
			const c32 cp = *it;
            const u32 glyphIdx = FT_Get_Char_Index( face, cp );
            const f32 advance = a_Font.GetAdvanceX( glyphIdx );

            if ( hasPrev )
            {
                if ( FT_HAS_KERNING( face ) )
                {
                    FT_Vector kerning;
                    if ( FT_Get_Kerning( face, prevGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                        prefixWidth += kerning.x / 64.f;
                }
                prefixWidth += a_Style.LetterSpacing;
            }

            prefixWidth  += advance;
            prevGlyphIdx  = glyphIdx;
            hasPrev       = true;

            // Candidate = prefix + LetterSpacing + crossKerning(last, '.') + "..."
            f32 crossKerning = 0.f;
            if ( FT_HAS_KERNING( face ) )
            {
                FT_Vector kerning;
                if ( FT_Get_Kerning( face, prevGlyphIdx, dotGlyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                    crossKerning = kerning.x / 64.f;
            }
            
            const f32 candidateWidth = prefixWidth + a_Style.LetterSpacing + crossKerning + ellipsisWidth;

            if ( candidateWidth <= a_MaxWidth ) 
            {
                // This prefix fits with the ellipsis, so save it as the best candidate so far and keep trying to fit more.
                bestPrefixByteCount = it.ByteIndex() + it.SequenceByteLength();
            }
            else
            {
                break; // Widths are non-decreasing - no longer prefix can fit.
            }
        }

        String result;
        Resize( result, bestPrefixByteCount );
        Reserve( result, bestPrefixByteCount + Size( c_Ellipsis ) );

        if ( bestPrefixByteCount > 0 )
            std::memcpy( Data( result ), Data( a_Line ), bestPrefixByteCount );

        for ( const char c : c_Ellipsis )
            PushBack( result, c );

        return result;
    }

} // namespace RatUI::FreeType::TextUtil
