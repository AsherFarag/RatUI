#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "Text.h"

namespace RatUI
{
    class ITextMetrics
    {
    public:
        virtual ~ITextMetrics() = default;

        /**
         * @brief Normalises whitespace, segments the text into atomic units, and pre-measures
         * each segment's pixel width.  The result should be cached and re-used across frames.
         * Only call this again when the text content or style changes.
         * @param a_Text  The text to prepare.
         * @param a_Style The text style that controls font, size, wrapping mode, letter spacing, etc.
         * @return A PreparedText value ready to pass to Measure() and to the renderer.
         */
        RATUI_NODISCARD virtual Optional<PreparedText> Prepare( StringView a_Text, const TextLayoutStyle& a_Style ) = 0;

        /**
		 * @brief Builds optimised render data (glyph quads, line metadata, etc.) from the prepared text and style.
         */
		RATUI_NODISCARD virtual Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize = { Limits<Unit>::max(), Limits<Unit>::max() } ) = 0;

        /**
         * @brief Rasterizes a single glyph as an MTSDF (RGBA8, 4 bytes per texel) and returns
         *        the pixel data owned by the implementation.
         *        The returned pointer is valid until the next call to RasterizeGlyph.
         *
         * @param a_Font       Font resource to rasterize from.
		 * @param a_GlyphIndex Glyph index to rasterize, as specified in the font's glyph table.
         * @param o_Pixels     Set to the RGBA8 pixel data (4 bytes per texel, row-major, Y-down).
         * @param o_Width      Set to the bitmap width in pixels (plane width + 2 * SDF padding).
         * @param o_Height     Set to the bitmap height in pixels (plane height + 2 * SDF padding).
         * @param o_Bearing    Set to the plane-bounds bearing: offset from baseline origin to the
         *                     plane-bounds top-left in base-size pixels (no SDF padding, Y-up).
         * @return true on success, false if the glyph could not be rasterized.
         */
        virtual bool RasterizeGlyph(
            FontHandle a_Font, GlyphID a_GlyphIndex, u32 a_FontSize,
            const Color*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2<FontUnit>& o_Bearing, FontUnit& o_XAdvance
        ) = 0;

    };

} // namespace RatUI