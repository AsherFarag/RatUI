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

        // TODO:

        virtual TextureHandle CreateTexture( TextureInfo a_Info, const void* a_Data ) = 0;
        virtual bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) = 0;
        virtual void DestroyTexture( TextureID a_Texture ) = 0;
        virtual bool IsValidTexture( TextureID a_Texture ) const = 0;
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