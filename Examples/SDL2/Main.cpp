#include "SDL2Application.h"
#include <RatUI/Backends/SDL2/SDL2Renderer.h>
#include <RatUI/Backends/FreeType/TextMetrics.h>
#include <RatUI/Widget/TextWidget.h>
#include <iostream>
#include <functional>
#include <filesystem>

#include "../Common/FeatureSandboxScene.h"
#include "../Common/DynamicTextScene.h"
#include "../Common/IDemoScene.h"

using namespace RatUI;
using namespace RatUI::Literals;

/**
 * @brief The RatUI Sandbox application.
 *
 * A scratch-pad for experimenting with RatUI widgets and layouts.
 * Add widgets to OnInitialize() and update/render them in OnUpdate()/OnRender().
 */
class SandboxApp : public SDL2Application
{
public:
    SandboxApp()
        : SDL2Application( { "RatUI Sandbox", Colorsf::Surface900, 1280, 720 } )
    {}

protected:

    DrawBatcher m_DrawBatcher;
    Unique<GlyphAtlas> m_Atlas;
    FreeType::FontCache m_FontCache;
    FreeType::TextMetrics m_TextMetrics{};

    std::unique_ptr<IDemoScene> m_Scene;
    WidgetID green;

    bool OnInitialize() override
    {
        const FontHandle fontHandle = { 1 };
        m_FontCache.RegisterFontHandle( fontHandle, "Resources/Fonts/Roboto-Medium.ttf" );
		m_TextMetrics.SetFontCache( &m_FontCache );
		//m_Renderer->SetFontCache( &m_FontCache );

        m_Scene = std::make_unique<DynamicTextScene>( fontHandle, &m_TextMetrics );
        m_Scene->Init();

        m_Atlas = MakeUnique<GlyphAtlas>( *m_Renderer, m_TextMetrics );
    
        return true;
    }

    void OnUpdate() override
    {
        static f32 prevTime = 0.f;
		const f32 currTime = SDL_GetTicks();
        const f32 deltaTime = ( currTime - prevTime ) / 1000.f;
        prevTime = currTime;

        if ( !m_Scene )
        {
            return;
        }

        m_Scene->Update( deltaTime );

        // Update the layout with the current window size as the available space.
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );
        m_Scene->GetScene().UpdateLayout( Vec2f{ static_cast<f32>( windowWidth ), static_cast<f32>( windowHeight ) } );
    }

    void OnRender( IRenderer& a_Renderer ) override
    {
        if ( !m_Scene )
        {
            return;
        }

		m_DrawBatcher.Clear();

        DrawList drawList{ m_DrawBatcher, *m_Atlas };
        m_Scene->Render( drawList );
        drawList.Finish();

        a_Renderer.Execute( m_DrawBatcher );
    }

    bool OnShutdown() override
    {
        m_Scene->Shutdown();
        m_Scene.reset();
        return true;
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        if ( m_Scene )
        {
            m_Scene->OnInputEvent( a_Event );
        }
    }
};

#undef main // SDL2 redefines main() on some platforms, so we undefine it here to avoid conflicts with our own main() function.

int main( int argc, char** argv )
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
