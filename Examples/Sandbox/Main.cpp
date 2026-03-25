#include <Application.h>
#include <SDL2Renderer.h>
#include "RatUI/RatUI.h"
#include <print>
#include <format>
#include <iostream>
#include <functional>

using namespace RatUI;

static SDL_Renderer* g_SDLRenderer = nullptr;

class RectWidget : public IWidget
{
public:
    RectWidget( const SDL_Color& a_Color )
        : Color( a_Color )
    {}

    SDL_Color Color{ 255, 255, 255, 255 };

    void OnPaint( Scene& a_Scene, const RenderContext& a_Context ) override
    {
        const LayoutNode* node = a_Scene.LayoutPool.Get( LayoutID );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;

        SDL_Rect sdlRect{
            (i32)rect.Origin[0],
            (i32)rect.Origin[1],
            (i32)rect.Size[0],
            (i32)rect.Size[1]
        };

        SDL_SetRenderDrawColor( g_SDLRenderer, Color.r, Color.g, Color.b, Color.a );
        SDL_RenderFillRect( g_SDLRenderer, &sdlRect );

        a_Scene.ForEachChildWidget( ID, [&]( IWidget& child )
        {
            child.OnPaint( a_Scene, a_Context );
		} );
    }
};

/**
 * @brief The RatUI Sandbox application.
 *
 * A scratch-pad for experimenting with RatUI widgets and layouts.
 * Add widgets to OnInitialize() and update/render them in OnUpdate()/OnRender().
 */
class SandboxApp : public Application
{
public:
    SandboxApp()
        : Application( { "RatUI Sandbox", 1280, 720 } )
    {}

protected:

    Scene m_Scene;

    bool OnInitialize() override
    {
        g_SDLRenderer = GetRenderer();

        // Root container
        WidgetID root = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 0, 0, 0, 0 } );
        m_Scene.RootWidget = root;

        LayoutNode* rootNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( root )->LayoutID );
        rootNode->Style.LayoutType = ELayoutType::Vertical;
        rootNode->Style.Spacing = 10.f;
        rootNode->Style.Padding = Edges{ 10.f };
        rootNode->Style.WidthMode = ESizingMode::Fill;
        rootNode->Style.HeightMode = ESizingMode::Fill;

        // ---------------- RED ----------------
        WidgetID red = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 255, 0, 0, 255 } );
        auto* redNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( red )->LayoutID );

        redNode->Style.FixedHeight = 100.f;
        redNode->Style.WidthMode = ESizingMode::Fill;
        redNode->Style.HeightMode = ESizingMode::Fixed;
        redNode->Style.Margin = Edges{ 10.f };

        rootNode->AddChild( *redNode );

        // ---------------- HBOX ----------------
        WidgetID hbox = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 0, 0, 50, 0 } );
        auto* hboxNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( hbox )->LayoutID );

        hboxNode->Style.LayoutType = ELayoutType::Horizontal;
        hboxNode->Style.Spacing = 10.f;
		hboxNode->Style.Padding = Edges{ 10.f };
        hboxNode->Style.HeightMode = ESizingMode::Fixed;
        hboxNode->Style.FixedHeight = 150.f;
        hboxNode->Style.WidthMode = ESizingMode::Fill;

        rootNode->AddChild( *hboxNode );

        // ---------------- GREEN ----------------
        WidgetID green = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 0, 255, 0, 0 } );
        auto* greenNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( green )->LayoutID );

        greenNode->Style.WidthMode = ESizingMode::Fill;
        greenNode->Style.HeightMode = ESizingMode::Fill;
        greenNode->Style.FlexGrow = 1.f;
        greenNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.

        hboxNode->AddChild( *greenNode );

        // ---------------- YELLOW ----------------
        WidgetID yellow = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 255, 255, 0, 0 } );
        auto* yellowNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( yellow )->LayoutID );

        yellowNode->Style.WidthMode = ESizingMode::Fill;
        yellowNode->Style.HeightMode = ESizingMode::Fill;
		yellowNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.
        yellowNode->Style.FlexGrow = 1.f;

        hboxNode->AddChild( *yellowNode );

        // ---------------- BLUE ----------------
        WidgetID blue = m_Scene.CreateWidget<RectWidget>( SDL_Color{ 0, 0, 255, 255 } );
        auto* blueNode = m_Scene.LayoutPool.Get( m_Scene.GetWidget( blue )->LayoutID );

        blueNode->Style.FixedHeight = 120.f;
        blueNode->Style.WidthMode = ESizingMode::Fill;
        blueNode->Style.HeightMode = ESizingMode::Fixed;

        rootNode->AddChild( *blueNode );

        return true;
    }

    void OnUpdate() override
    {
        // TODO: Update the RatUI widget tree
    }

    void OnRender() override
    {
        using namespace RatUI;

        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );

        const Vec2f size{ (f32)windowWidth, (f32)windowHeight };

        // Clear
        SDL_SetRenderDrawColor( g_SDLRenderer, 255, 255, 255, 255 );
        SDL_RenderClear( g_SDLRenderer );

        // Layout
        m_Scene.UpdateLayout( size );

        // Render
        RenderContext ctx{};
        m_Scene.Render( ctx );
    }

    bool OnShutdown() override
    {
        m_Renderer.reset();
        return true;
    }

private:
    std::unique_ptr<SDL2Renderer> m_Renderer;
};

#undef main // SDL2 redefines main() on some platforms, so we undefine it here to avoid conflicts with our own main() function.

int main( int argc, char** argv )
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
