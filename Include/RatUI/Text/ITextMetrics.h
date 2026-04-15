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
        virtual Optional<PreparedText> Prepare( StringView a_Text, const TextLayoutStyle& a_Style ) = 0;

        /**
		 * @brief Builds optimised render data (glyph quads, line metadata, etc.) from the prepared text and style.
         */
		virtual Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2f a_MaxSize = { Limits<f32>::max(), Limits<f32>::max() } ) = 0;

        /**
         * @brief Rasterizes a single glyph as an MSDF (RGB8, 3 bytes per texel) and returns
         *        the pixel data owned by the implementation.
         *        The returned pointer is valid until the next call to RasterizeGlyph.
         *
         * @param a_Font       Font resource to rasterize from.
		 * @param a_GlyphIndex The index of the glyph to rasterize (not the Unicode codepoint).
         * @param o_Pixels     Set to the RGBA8 pixel data (4 bytes per texel, row-major, Y-down).
         * @param o_Width      Set to the bitmap width in pixels.
         * @param o_Height     Set to the bitmap height in pixels.
         * @param o_Bearing    Set to the glyph bearing (offset from baseline origin to bitmap
         *                        top-left, Y-up convention matching FreeType).
         * @return true on success, false if the glyph could not be rasterized.
         */
        virtual bool RasterizeGlyph(
            FontHandle a_Font, u32 a_GlyphIndex, u32 a_FontSize,
            const Coloru8*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2i& o_Bearing
        ) = 0;

    };

} // namespace RatUI