#pragma once
#include <RatUI/Renderer/Renderer.h>
#include <SDL2/SDL.h>

/**
 * @brief A RatUI IRenderer implementation backed by SDL2.
 *
 * Wraps an SDL_Renderer and exposes it for use within the RatUI rendering
 * pipeline.  Widgets that require low-level drawing access can obtain the
 * underlying handle via GetSDLRenderer().
 */
class SDL2Renderer : public RatUI::IRenderer
{
public:
    explicit SDL2Renderer( SDL_Renderer* a_Renderer ) : m_Renderer( a_Renderer ) {}

    /** @brief Returns the underlying SDL_Renderer handle. */
    SDL_Renderer* GetSDLRenderer() const { return m_Renderer; }

private:
    SDL_Renderer* m_Renderer;
};
