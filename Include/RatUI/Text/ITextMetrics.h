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
    };

} // namespace RatUI