#include <Application.h>
#include <SDL2Renderer.h>
#include "RatUI/RatUI.h"
#include <print>
#include <format>
#include <iostream>

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
    bool OnInitialize() override
    {
        //m_Renderer = std::make_unique<SDL2Renderer>( GetRenderer() );
        return true;
    }

    void OnUpdate() override
    {
        // TODO: Update the RatUI widget tree
    }

    void OnRender() override
    {
		using namespace RatUI;

        // Root (vertical stack)
        LayoutNode rootWidget;
        rootWidget.Style.LayoutType = ELayoutType::Vertical;
        rootWidget.Style.Spacing = 10.f;
        rootWidget.Style.WidthMode = ESizingMode::Content;
        rootWidget.Style.HeightMode = ESizingMode::Content;
		rootWidget.Style.Padding = Edges{ 10.f };

        // ---------------- RED ----------------
        LayoutNode redWidget;
        redWidget.Style.FixedHeight = 100.f;
        redWidget.Style.WidthMode = ESizingMode::Fill;
        redWidget.Style.HeightMode = ESizingMode::Fixed;
        redWidget.Style.PercentWidth = 1.f;
        redWidget.Style.Margin = Edges{ 10.f };
        rootWidget.AddChild( redWidget );

        // ---------------- HBOX (replaces green) ----------------
        LayoutNode hbox;
        hbox.Style.LayoutType = ELayoutType::Horizontal;
        hbox.Style.Spacing = 10.f;
        hbox.Style.WidthMode = ESizingMode::Fill;
        hbox.Style.HeightMode = ESizingMode::Fixed;
        hbox.Style.FixedHeight = 150.f; // controls row height
        hbox.Style.PercentWidth = 1.f;
        rootWidget.AddChild( hbox );

        // ---------------- GREEN ----------------
        LayoutNode greenWidget;
        greenWidget.Style.WidthMode = ESizingMode::Fill;
        greenWidget.Style.HeightMode = ESizingMode::Fill;
        greenWidget.Style.PercentWidth = 0.5f; // share space
        hbox.AddChild( greenWidget );

        // ---------------- YELLOW ----------------
        LayoutNode yellowWidget;
        yellowWidget.Style.WidthMode = ESizingMode::Fill;
        yellowWidget.Style.HeightMode = ESizingMode::Fill;
        yellowWidget.Style.PercentWidth = 0.5f; // other half
        hbox.AddChild( yellowWidget );

        // ---------------- BLUE ----------------
        LayoutNode blueWidget;
        blueWidget.Style.FixedHeight = 120.f;
        blueWidget.Style.WidthMode = ESizingMode::Fill;
        blueWidget.Style.HeightMode = ESizingMode::Fill;
        blueWidget.Style.PercentWidth = 1.f;
        rootWidget.AddChild( blueWidget );

        // ---------------- ANIMATION ----------------
        f32 time = static_cast<f32>( SDL_GetTicks() ) / 1000.f;

        // Animate the HBOX height instead of green directly
        hbox.Style.FixedHeight = 50 + 150 * ( 0.5f + 0.5f * std::sin( time ) );

        // Animate how much width green takes vs yellow
        greenWidget.Style.FlexGrow = 0.5f + 0.5f * std::sin( time * 0.5f );
        yellowWidget.Style.FlexGrow = 2.f;

        // ---------------- LAYOUT PASS ----------------
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );

        const Vec2f availableSize{ (f32)windowWidth, (f32)windowHeight };
        MeasureLayoutNode( rootWidget, availableSize );

        const Rectf rootRect{ .Origin = Vec2f{ 0.f, 0.f }, .Size = availableSize };
        ArrangeLayoutNode( rootWidget, rootRect );

        // ---------------- RENDER ----------------
        SDL_Renderer* sdl = GetRenderer();

        SDL_SetRenderDrawColor( sdl, 255, 255, 255, 255 );
        SDL_Rect d{ 0,0, windowWidth, windowHeight };
        SDL_RenderFillRect( sdl, &d );

        // RED
        SDL_SetRenderDrawColor( sdl, 255, 0, 0, 255 );
        SDL_Rect redRect{
            (i32)redWidget.Layout.FinalRect.Origin[0],
            (i32)redWidget.Layout.FinalRect.Origin[1],
            (i32)redWidget.Layout.FinalRect.Size[0],
            (i32)redWidget.Layout.FinalRect.Size[1]
        };
        SDL_RenderFillRect( sdl, &redRect );

        // GREEN
        SDL_SetRenderDrawColor( sdl, 0, 255, 0, 255 );
        SDL_Rect greenRect{
            (i32)greenWidget.Layout.FinalRect.Origin[0],
            (i32)greenWidget.Layout.FinalRect.Origin[1],
            (i32)greenWidget.Layout.FinalRect.Size[0],
            (i32)greenWidget.Layout.FinalRect.Size[1]
        };
        SDL_RenderFillRect( sdl, &greenRect );

        // YELLOW
        SDL_SetRenderDrawColor( sdl, 255, 255, 0, 255 );
        SDL_Rect yellowRect{
            (i32)yellowWidget.Layout.FinalRect.Origin[0],
            (i32)yellowWidget.Layout.FinalRect.Origin[1],
            (i32)yellowWidget.Layout.FinalRect.Size[0],
            (i32)yellowWidget.Layout.FinalRect.Size[1]
        };
        SDL_RenderFillRect( sdl, &yellowRect );

        // BLUE
        SDL_SetRenderDrawColor( sdl, 0, 0, 255, 255 );
        SDL_Rect blueRect{
            (i32)blueWidget.Layout.FinalRect.Origin[0],
            (i32)blueWidget.Layout.FinalRect.Origin[1],
            (i32)blueWidget.Layout.FinalRect.Size[0],
            (i32)blueWidget.Layout.FinalRect.Size[1]
        };
        SDL_RenderFillRect( sdl, &blueRect );

        static bool once = false;
        if ( !once )
        {
			once = true;
			std::cout << std::format( "Root widget rect: Origin({:.1f}, {:.1f}), Size({:.1f}, {:.1f})\n",
				rootWidget.Layout.FinalRect.Origin[0], rootWidget.Layout.FinalRect.Origin[1],
				rootWidget.Layout.FinalRect.Size[0], rootWidget.Layout.FinalRect.Size[1] );

            std::cout << std::format( "Red widget rect: Origin({:.1f}, {:.1f}), Size({:.1f}, {:.1f})\n",
                redWidget.Layout.FinalRect.Origin[ 0 ], redWidget.Layout.FinalRect.Origin[ 1 ],
				redWidget.Layout.FinalRect.Size[0], redWidget.Layout.FinalRect.Size[1] );

            std::cout << std::format( "Green widget rect: Origin({:.1f}, {:.1f}), Size({:.1f}, {:.1f})\n",
				greenWidget.Layout.FinalRect.Origin[0], greenWidget.Layout.FinalRect.Origin[1],
				greenWidget.Layout.FinalRect.Size[0], greenWidget.Layout.FinalRect.Size[1] );

			std::cout << std::format( "Blue widget rect: Origin({:.1f}, {:.1f}), Size({:.1f}, {:.1f})\n",
				blueWidget.Layout.FinalRect.Origin[0], blueWidget.Layout.FinalRect.Origin[1],
				blueWidget.Layout.FinalRect.Size[0], blueWidget.Layout.FinalRect.Size[1] );
        }
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
