#pragma once
#include "../Core.h"

namespace RatUI
{
    struct SolidBrush
    {
        Color Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct GradientBrush
    {
    };

    struct TextureBrush
    {
    };

    /**
     * @brief Brush is a type that encapsulates the information needed to fill shapes with color, gradients, or textures.
     */
	using Brush = Variant<SolidBrush, GradientBrush, TextureBrush>;
}