#pragma once
#include "../Core.h"
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
        virtual PreparedText Prepare( TextView a_Text, const TextStyle& a_Style ) = 0;

        /**
         * @brief Computes a TextMeasurement (total size, baseline, line count) for a block of
         * previously prepared text, constrained to the given maximum line width.
         * @param a_Prepared  A PreparedText produced by Prepare().
         * @param a_Style     The same text style that was used when calling Prepare().
         * @param a_MaxWidth  Maximum line width in pixels. Use Limits<f32>::max() for no wrapping.
         * @return The measured size and metadata of the text block.
         */
        virtual TextMeasurement Measure( const PreparedText& a_Prepared, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) = 0;
    };

} // namespace RatUI