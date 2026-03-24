#pragma once
#include "../Core.h"

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
	using Brush = Variant<SolidBrush, LinearGradientBrush, TextureBrush>;
}