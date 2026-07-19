#pragma once
#include "../Core.h"

namespace RatUI
{
    class IRenderer;

    /**
     * @brief TextureHandle is an opaque, user-defined handle that represents a texture resource managed by the renderer backend.
     */
    using TextureHandle = Shared<void>;

    /**
     * @brief Defines the dimensions of the corners and edges for nine-slice scaling of textures.
     */
    struct NineSlice
    {
        u16 Left{ 0 };   ///< The number of pixels from the left edge of the texture that defines the left border region.
        u16 Top{ 0 };    ///< The number of pixels from the top edge of the texture that defines the top border region.
        u16 Right{ 0 };  ///< The number of pixels from the right edge of the texture that defines the right border region.
        u16 Bottom{ 0 }; ///< The number of pixels from the bottom edge of the texture that defines the bottom border region.

		Vec2f Scale{ 1.f, 1.f }; ///< Scale factor applied to the entire nine-slice image. 
                                 ///< This allows for resizing the image while maintaining the proportions of the corners and edges.

        // TODO: Support tiling?
        // bool TileCenter : 1 = false; ///< If true, the center region will be tiled instead of stretched.
        // bool TileX      : 1 = false; ///< If true, the left and right regions will be tiled instead of stretched.
        // bool TileY      : 1 = false; ///< If true, the top and bottom regions will be tiled instead of stretched.
    };

    /**
     * @brief Supported texture formats for renderer backends.
     */
    enum class ETextureFormat : u8
    {
        Unknown = 0,

        R8,
        RG8,
        RGB8,
        RGBA8,
    };

    /**
	 * @brief Describes the filtering mode used when sampling a texture, which determines how the texture is sampled when it is scaled up or down.
     */
    enum class ETextureFilter : u8
    {
        Nearest,
        Linear,
    };

    /**
	 * @brief Parameters that define how a texture should be sampled when rendering.
     */
    struct TextureSampler
    {
		ETextureFilter Filter{ ETextureFilter::Linear };
    };

    /**
	 * @brief Information about a texture resource, including its size, format, and sampling parameters.
	 * Used for creating textures and querying texture properties from the renderer.
     */
    struct TextureInfo
    {
		Vec2u          Size;    ///< The width and height of the texture in pixels.
		ETextureFormat Format;  ///< The pixel format of the texture, which determines how the texture data is interpreted and stored in memory.
		TextureSampler Sampler; ///< The sampling parameters for the texture, such as filtering mode.
    };

} // namespace RatUI