#pragma once
#include "../Example.h"

struct SDL_Window;
namespace RatUI::OpenGL   { class OpenGLRenderer; }

/**
 * @brief Runs an Example in an SDL2 window with the OpenGL renderer.
 *
 * Opens the window, loads the fonts, turns SDL events into RatUI input and lays out and
 * draws the example's scene every frame. Also builds for the browser via Emscripten.
 */
class Application
{
public:
    Application();
    ~Application();

    /** @brief Runs TExample until the window is closed. In the browser this never returns. */
    template<typename TExample>
    bool Run()
    {
        return Run( []( TextMetrics& a_TextMetrics ) -> Unique<Example> { return MakeUnique<TExample>( a_TextMetrics ); } );
    }

private:
    using ExampleFactory = Unique<Example>( * )( TextMetrics& );

    bool Run( ExampleFactory a_MakeExample );
    bool Initialize();
    void LoadFonts();
    void Shutdown();
    void Frame();
    void ProcessEvents();

    bool        m_Running  { true };
    bool        m_DebugDraw{ false }; ///< Widget bounds overlay, toggled with F1 (debug builds only).
    u64         m_LastTicks{ 0 };
    SDL_Window* m_Window   { nullptr };
    void*       m_GLContext{ nullptr }; ///< SDL_GLContext

    Unique<OpenGL::OpenGLRenderer> m_Renderer;
    FontLibrary                    m_Fonts;
    Unique<TextMetrics>            m_TextMetrics;
    Unique<GlyphAtlas>             m_Atlas;
    Unique<DrawList>               m_DrawList;
    Unique<Example>                m_Example;
};
