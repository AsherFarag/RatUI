#pragma once
#include "../Core.h"

#ifndef RATUI_CUSTOM_BRUSH_IMPL
    ::RatUI::Variant<::RatUI::SolidBrush, ::RatUI::LinearGradientBrush, ::RatUI::TextureBrush>;
#endif // Default to a simple variant of some common brush types if no custom brush implementation is provided.

namespace RatUI
{
    struct SolidBrush
    {
        Color Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct LinearGradientBrush
    {
        struct GradientStop
        {
            Color Color;
            f32 Position; // 0.0 to 1.0
        };

        Array<GradientStop> Colors;
        f32 Angle; // TODO: Add a Radians and Degrees type
    };

    struct TextureBrush
    {
    };

    /**
     * @brief Brush is a type that encapsulates the information needed to fill shapes with color, gradients, or textures.
     */
    using Brush = RATUI_CUSTOM_BRUSH_IMPL;
}