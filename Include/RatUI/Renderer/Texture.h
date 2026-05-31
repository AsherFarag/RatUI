#pragma once
#include "../Core.h"

namespace RatUI
{
    class IRenderer;

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

    /**
     * @brief An opaque identifier for a texture resource.
     * @example OpenGL: GLuint; SDL2: SDL_Texture*; etc.
     */
    struct TextureID 
    { 
        union
        {
            void* Ptr;
            uptr  ID{ 0 };
        };

        constexpr bool operator==( const TextureID& a_Other ) const { return ID == a_Other.ID; } 

        static constexpr TextureID Null() { return TextureID{}; }
    };

    /**
     * @brief A texture resource managed by the renderer.
     * The Texture class is a RAII wrapper around a TextureID, 
     * ensuring that the texture is properly released when the Texture object goes out of scope.
     */
    class Texture
    {
    public:
        Texture( IRenderer& a_Renderer, TextureID a_ID )
            : Renderer( a_Renderer )
            , ID( a_ID )
        {}

        // TODO: Define in Texture.cpp now that we are no longer header only
        ~Texture(); ///< Defined in 'IRenderer.h' to avoid circular dependency.

        IRenderer& Renderer;
        TextureID  ID;
    };

    /**
     * @brief A handle to a texture resource that can be safely passed around and copied.
     * The TextureHandle class manages a shared pointer to a Texture object, 
     * allowing for reference counting and automatic cleanup of the underlying texture resource when no longer needed.
     */
    class TextureHandle
    {
    public:
        TextureHandle() = default;
		TextureHandle( Shared<class Texture> a_Texture ) : m_Texture( std::move( a_Texture ) ) {}

        /** @brief Check if this handle currently owns a valid texture. */
        bool IsValid() const { return m_Texture != nullptr; }

        /** @brief Get the underlying TextureID, or TextureID::Null() if this handle is empty. */
        TextureID GetID() const { return m_Texture ? m_Texture->ID : TextureID::Null(); }

		/** @brief Get the texture information (size, format, etc.) for the associated texture, if valid. */
		Optional<TextureInfo> QueryInfo() const; ///< Defined in 'IRenderer.h' to avoid circular dependency.

        /** @brief Reset the handle, releasing the associated texture. */
        void Reset() { m_Texture.reset(); }

        /** @brief Get a null texture handle. */
        static TextureHandle Null() { return TextureHandle(); }

        bool operator==( const TextureHandle& a_Other ) const { return m_Texture == a_Other.m_Texture; }
        operator bool() const { return IsValid(); }

    private:
        Shared<class Texture> m_Texture;
    };

} // namespace RatUI