#include "SDL2Application.h"
#include <RatUI/Backends/FreeType/TextMetrics.h>
#include <RatUI/Widget/TextWidget.h>
#include <iostream>
#include <functional>
#include <filesystem>

#include "../Common/FeatureSandboxScene.h"
#include "../Common/DynamicTextScene.h"

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
        : SDL2Application( { "RatUI Sandbox", ToColorF32( Colors::Surface900 ), 1280, 720 } )
    {}

protected:

    Unique<DrawList> m_DrawList;
    Unique<GlyphAtlas> m_Atlas;
    FreeType::FontCache m_FontCache;
    FreeType::TextMetrics m_TextMetrics{};

    Unique<IDemoScene> m_Scene;

	f32 m_UIScale{ 1.f };

    bool OnInitialize() override
    {
        const FontHandle fontHandle = { 1 };
        m_FontCache.RegisterFontHandle( fontHandle, "Resources/Fonts/Roboto-Medium.ttf" );
		m_TextMetrics.SetFontCache( &m_FontCache );

        m_Scene = MakeUnique<FeatureSandboxScene>( fontHandle, &m_TextMetrics );
        m_Scene->Init();

        m_Atlas = MakeUnique<GlyphAtlas>( *m_Renderer, m_TextMetrics );
		m_DrawList = MakeUnique<DrawList>( *m_Atlas );
    
        return true;
    }

    void OnUpdate() override
    {
        static f32 prevTime = 0.f;
		const f32 currTime = SDL_GetTicks();
        const f32 deltaTime = ( currTime - prevTime ) / 1000.f;
        prevTime = currTime;

		//m_UIScale = 1.f + 0.5f * std::sin( currTime / 1000.f ); // Oscillate UI scale between  and 1.5 over time.

        if ( !m_Scene )
        {
            return;
        }

        m_Scene->Update( deltaTime );

        // Update the layout with the current window size as the available space.
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );
        Vec2<Unit> windowSize{ Unit{ static_cast<f32>( windowWidth ) }, Unit{ static_cast<f32>( windowHeight ) } };

		windowSize[0] /= m_UIScale;
		windowSize[1] /= m_UIScale;

        m_Scene->GetScene().UpdateLayout( windowSize );
    }

    void OnRender( IRenderer& a_Renderer ) override
    {
        if ( !m_Scene )
        {
            return;
        }

		// Get dpi scale for current window (for correct text rendering on high-DPI displays)
        f32 dpiscale = 1.f;
        SDL_GetDisplayDPI( SDL_GetWindowDisplayIndex( GetWindow() ), nullptr, &dpiscale, nullptr );
		dpiscale /= 96.f; // Convert from DPI to scale factor (assuming 96 DPI is the baseline)
        m_DrawList->SetDPIScale( dpiscale * m_UIScale );

        m_DrawList->Clear();
		m_Scene->Render( *m_DrawList );
		m_DrawList->Flush( a_Renderer );
    }

    bool OnShutdown() override
    {
		m_DrawList->Clear();
        m_DrawList.reset();

        m_Scene->Shutdown();
        m_Scene.reset();

		m_Atlas.reset();
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
