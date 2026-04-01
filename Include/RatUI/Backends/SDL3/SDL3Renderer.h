#pragma once
#include "../../Include/RatUI/RatUI.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

class SDL2Renderer : public RatUI::IRenderer
{
    using namespace RatUI;
public:
    SDL2Renderer( SDL_Renderer& a_Renderer )
        : m_Renderer( a_Renderer )
    {}

    void Execute( Span<const struct DrawCmd> a_Commands ) override;

protected:
    SDL_Renderer& m_Renderer;
};

inline void SDL2Renderer::Execute( Span<const DrawCmd> a_Commands )
{
    for ( const DrawCmd& cmd : a_Commands )
    {
    }
}