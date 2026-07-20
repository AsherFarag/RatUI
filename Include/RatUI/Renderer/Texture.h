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

    struct TextureView
    {
        TextureHandle Handle;
        Rect<u16>     Region{ Rect<u16>::Infinite() }; ///< The region of the texture to be used for rendering. Defaults to the entire texture.

        bool operator==( const TextureView& a_Other ) const = default;
    };

    /**
     * @brief Helper function to compute the UV coordinates for a given texture region and full texture size.
     * The UV coordinates are normalized texture coordinates in the range [0, 1], 
     * where (0, 0) corresponds to the top-left corner of the texture and (1, 1) corresponds to the bottom-right corner.
     * @param a_Region The pixel region of the texture to be used for rendering, as specified in a TextureView.
     * @param a_FullTextureSize The size, in pixels, of the full underlying texture that the view references.
     * @return A Rect<f32> whose Origin/Size describe the UV sub-rect for a_Region.
     */
    template<typename T>
    inline Rect<f32> ComputeUVRect( Rect<T> a_Region, Vec2u a_FullTextureSize )
    {
        if ( a_Region.IsInfinite() || a_FullTextureSize[0] == 0 || a_FullTextureSize[1] == 0 )
            return Rect<f32>{ Vec2f{ 0.f, 0.f }, Vec2f{ 1.f, 1.f } };

        const f32 rcpW = 1.f / static_cast<f32>( a_FullTextureSize[0] );
        const f32 rcpH = 1.f / static_cast<f32>( a_FullTextureSize[1] );

        return Rect<f32>{
            Vec2f{ static_cast<f32>( a_Region.Origin[0] ) * rcpW, static_cast<f32>( a_Region.Origin[1] ) * rcpH },
            Vec2f{ static_cast<f32>( a_Region.Size[0] )   * rcpW, static_cast<f32>( a_Region.Size[1] )   * rcpH }
        };
    }

} // namespace RatUI