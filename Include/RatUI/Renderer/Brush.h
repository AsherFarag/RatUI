#pragma once
#include "../Core.h"

namespace RatUI
{
    // TODO: This would be useful, find a clean and easy way to implement it in user renderers

    struct SolidBrush
    {
        Color Fill{ 1.0f, 1.0f, 1.0f, 1.0f };
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