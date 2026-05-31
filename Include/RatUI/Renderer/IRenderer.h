#pragma once
#include "../Core.h"
#include "Texture.h"

namespace RatUI
{
	class IRenderer;

    /** 
     * @brief IRenderer is a user-defined interface for the basic drawing operations required by the UI elements.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

		/** @brief Executes the given draw batches, each DrawBatch should equate to a single draw call on the backend. */
		virtual void Execute( const struct DrawBatcher& a_Batcher ) = 0;

        /**
         * @brief Creates a texture resource on the GPU with the specified information and initial data.
         * @param a_Info The information about the texture to create. @a_Data will be interpreted according to the format specified in a_Info.
         * @param a_Data A pointer to the initial pixel data for the texture. 
         *               The layout and format of this data should match the specifications in a_Info (e.g. width, height, format).
         * @return A TextureHandle that can be used to reference the created texture in subsequent draw calls. 
         *         The handle will manage the lifetime of the texture resource.
         */ 
        virtual TextureHandle CreateTexture( TextureInfo a_Info, const void* a_Data ) = 0;

        /**
         * @brief Updates a region of an existing texture resource with new pixel data.
         * @param a_Texture The TextureID of the texture to update. Must be a valid texture that was previously created and not yet destroyed.
         * @param a_MipLevel The mipmap level to update, where 0 is the base level. Must be less than the total mip levels of the texture.
         * @param a_Region The rectangular region of the texture to update, specified in pixels relative to the top-left corner of the texture.
         * @param a_Data A pointer to the new pixel data for the specified region.
         * @param a_DataSizeBytes The size of the data pointed to by a_Data in bytes.
         * @return true if the update was successful, false if the texture ID was invalid or the parameters were out of bounds.
         */
        virtual bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) = 0;

        /**
         * @brief Destroys a texture resource, freeing its associated GPU memory. After this call, the texture ID is no longer valid and should not be used in draw calls.
         * @param a_Texture The TextureID of the texture to destroy. Must be a valid texture that was previously created and not yet destroyed.
         */
        virtual void DestroyTexture( TextureID a_Texture ) = 0;

        /**
         * @brief Checks if a given TextureID corresponds to a valid texture resource that is currently managed by the renderer.
         * @param a_Texture The TextureID to validate.
         * @return true if the texture ID is valid and corresponds to an existing texture resource, false otherwise.
         */
        virtual bool IsValidTexture( TextureID a_Texture ) const = 0;

        /**
         * @brief Retrieves information about a texture resource, such as its size, format, and sampler settings.
         * @param a_Texture The TextureID of the texture to query. Must be a valid texture that was previously created and not yet destroyed.
         * @return An optional TextureInfo structure containing the texture's information if the texture ID is valid, or NullOpt if the texture ID is invalid.
         * @note This function may involve a lookup in the renderer's internal texture management system, so it may not be as fast as using cached information.
         */
        virtual Optional<TextureInfo> QueryTextureInfo( TextureID a_Texture ) const = 0;
    };

	inline Texture::~Texture()
    {
        if ( ID != TextureID::Null() )
            Renderer.DestroyTexture( ID );
    }

    inline Optional<TextureInfo> RatUI::TextureHandle::QueryInfo() const
    {
		return m_Texture ? m_Texture->Renderer.QueryTextureInfo( m_Texture->ID ) : Optional<TextureInfo>{};
    }
    
} // namespace RatUI