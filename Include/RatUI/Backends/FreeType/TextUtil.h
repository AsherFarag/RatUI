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
    Unit MeasureLineWidth(
        Font& a_Font,
        StringView a_Line,
        const TextLayoutStyle& a_Style );

    /**
     * @brief Applies the specified text transformation (uppercase, lowercase, capitalize) to a UTF-8 encoded string.

      * TODO: Support locale-aware transformations and proper word-boundary detection for capitalization.
      *       Probably need to use ICU. I miss ascii.
     */
    String ApplyTextTransform( String&& a_Text, ETextTransform a_Transform );

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
    bool TruncateShapedLineWithEllipsis(
        ShapedText&       o_Text,
        u32               a_LineIndex,
        Unit              a_MaxWidth,
        const ShapedText& a_Ellipsis );

} // namespace RatUI::FreeType::TextUtil
