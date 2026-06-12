#pragma once
#include "../Core.h"
#include "Texture.h"

namespace RatUI
{
    // TODO: Implement brushes

    struct SolidBrush
    {
        Color Fill{ Colors::White };

        SolidBrush& WithFill( Color a_Fill ) { Fill = a_Fill; return *this; }
    };

    struct TextureBrush
    {
        TextureHandle Texture{};
        Color         Tint{ Colors::White };

        TextureBrush& WithTexture( TextureHandle a_Texture ) { Texture = std::move( a_Texture ); return *this; }
        TextureBrush& WithTint( Color a_Tint ) { Tint = a_Tint; return *this; }
    };

    struct NineSliceBrush
    {
        TextureHandle Texture{};
        Color         Tint{ Colors::White };
        NineSlice     Slice{};

        NineSliceBrush& WithTexture( TextureHandle a_Texture ) { Texture = std::move( a_Texture ); return *this; }
        NineSliceBrush& WithTint( Color a_Tint ) { Tint = a_Tint; return *this; }
        NineSliceBrush& WithSlice( NineSlice a_Slice ) { Slice = a_Slice; return *this; }
    };

    /**
     * @brief Brush is a type that encapsulates the information needed to fill shapes with color, gradients, or textures.
     */
	using Brush = Variant<
        SolidBrush, 
        TextureBrush, 
        NineSliceBrush
    >;
}