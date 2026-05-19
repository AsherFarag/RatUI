#pragma once
#include "../Core.h"

namespace RatUI
{
    class IRenderer;

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