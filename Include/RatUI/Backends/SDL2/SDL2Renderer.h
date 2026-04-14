#pragma once
#include "../../RatUI.h"
#include <SDL.h>
//#include <SDL_ttf.h>
//#include <SDL_image.h>

class SDL2Renderer : public RatUI::IRenderer
{
public:
    SDL2Renderer( SDL_Renderer* a_Renderer = nullptr )
        : m_Renderer( a_Renderer )
    {}

    void SetSDLRenderer( SDL_Renderer* a_Renderer )
    {
        m_Renderer = a_Renderer;
    }

	void Execute( const RatUI::DrawBatcher& a_Batcher ) override;

    // TODO: Currently only handles rounding the same for all corners
    void RenderFillRoundedRect( RatUI::Rectf a_Rect, RatUI::CornerRounding a_Rounding, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform );

    // TODO: Currently only handles rounding the same for all corners
    void RenderFillRoundedRectBorder( RatUI::Rectf a_Rect, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform );

    void RenderFillCircle( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform );

    void RenderFillCircleBorder( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform );

    // TODO: These are implemented in branch/text but not yet merged
    RatUI::Optional<RatUI::TextureID> CreateTexture( RatUI::u32 a_Width, RatUI::u32 a_Height, RatUI::ETextureFormat a_Format, const void* a_Data ) { return RatUI::NullOpt; }
    bool UpdateTexture( RatUI::TextureID a_Texture, RatUI::u32 a_MipLevel, RatUI::Rectu a_Region, const void* a_Data, RatUI::size a_DataSizeBytes ) { return false; }
    void DestroyTexture( RatUI::TextureID a_Texture ) {}
    bool IsValidTexture( RatUI::TextureID a_Texture ) const { return false; }

    static SDL_Color ToSDLColor( RatUI::Colorf a_Color )
    {
        return SDL_Color{
            static_cast<Uint8>( a_Color[0] * 255.f ),
            static_cast<Uint8>( a_Color[1] * 255.f ),
            static_cast<Uint8>( a_Color[2] * 255.f ),
            static_cast<Uint8>( a_Color[3] * 255.f )
        };
    }

protected:
    SDL_Renderer* m_Renderer;
    RatUI::Array<int> m_IndexBuffer;
};

inline void SDL2Renderer::Execute( const RatUI::DrawBatcher& a_Batcher )
{
    using namespace RatUI;

    Clear( m_IndexBuffer );
    Resize( m_IndexBuffer,  Size( a_Batcher.Indices ) );

    // TODO: Is there a way sdl can use u16 indices?
    for ( size i = 0; i < Size( a_Batcher.Indices ); ++i )
        m_IndexBuffer[i] = static_cast<int>( a_Batcher.Indices[i] );

    // Note: We're allowed to do this because our Vertex struct is laid out in memory exactly like SDL_Vertex, 
    // so we can treat the DrawBatcher's vertex array as an array of SDL_Vertex without needing to transform it first.
    // TODO: Add some static_asserts to ensure that this is always the case.
    const SDL_Vertex* vertexData = reinterpret_cast<const SDL_Vertex*>( Data( a_Batcher.Vertices ) );

    for ( const auto& batch : a_Batcher.Batches )
    {
        if ( HasValue( batch.ClipRect ) )
        {
            const SDL_Rect sdlClipRect{
                static_cast<int>( batch.ClipRect->Origin[0] ),
                static_cast<int>( batch.ClipRect->Origin[1] ),
                static_cast<int>( batch.ClipRect->Size[0] ),
                static_cast<int>( batch.ClipRect->Size[1] )
            };
            SDL_RenderSetClipRect( m_Renderer, &sdlClipRect );
        }
        else
        {
            SDL_RenderSetClipRect( m_Renderer, nullptr );
        }

        // TODO: We need to use the batch.Transform matrix here but we need SDL_GPU or something

        if ( IsValidTexture( batch.Texture ) )
        {
        }
        else
        {
            SDL_SetTextureColorMod( nullptr, 255, 255, 255 );
            SDL_SetTextureAlphaMod( nullptr, 255 );

            SDL_RenderGeometry( m_Renderer, nullptr,
                                vertexData, static_cast<int>( Size( a_Batcher.Vertices ) ),
                                Data( m_IndexBuffer )  + batch.IndexOffset, batch.IndexCount );
        }
    }
}