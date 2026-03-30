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
         * @brief Measures a string of text with the given style, optionally constrained to a maximum width for wrapping.
         * @param a_Text The text to measure.
         * @param a_Style The text style to use for measurement.
         * @param a_MaxWidth Maximum width before wrapping. Use Limits<f32>::max() for no wrapping.
         * @return The measured size and metadata of the text.
         */
        virtual TextMeasurement Measure( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) = 0;

        /**
         * @brief Shapes a string of text into a backend-owned ShapedText object.
         * The returned ShapedText is valid until DestroyShapedText is called.
         * @param a_Text The text to shape.
         * @param a_Style The text style to use.
         * @param a_MaxWidth Maximum width for wrapping.
         * @return A ShapedText handle.
         */
        virtual ShapedText Shape( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) = 0;

        /** @brief Releases a previously created ShapedText object, freeing any associated resources. */
        virtual void ReleaseShapedText( const ShapedText& a_ShapedText ) = 0;
    };

} // namespace RatUI