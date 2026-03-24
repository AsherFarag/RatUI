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

        // Demo of a vertical stack of 3 rectangles with different colors, sizes, and margins
        Widget rootWidget;
        rootWidget.Style.LayoutType = ELayoutType::Vertical;
        rootWidget.Style.Spacing = 10.f;
		//rootWidget.Style.Padding = Edges::Uniform( 10.f );
		//rootWidget.Style.Margin = Edges::Uniform( 20.f );
        rootWidget.Style.WidthMode = ESizingMode::Content;
        rootWidget.Style.HeightMode = ESizingMode::Content;

        Widget redWidget;
        redWidget.Style.FixedHeight = 100.f;
        redWidget.Style.WidthMode = ESizingMode::Fill;
		redWidget.Style.HeightMode = ESizingMode::Fixed;
        redWidget.Style.PercentWidth = 1.f;
		redWidget.Style.Margin = Edges{ 10.f };
        rootWidget.AddChild( redWidget );

        Widget greenWidget;
        greenWidget.Style.FixedHeight = 150.f;
        greenWidget.Style.WidthMode = ESizingMode::Fill;
		greenWidget.Style.HeightMode = ESizingMode::Fixed;
        greenWidget.Style.PercentWidth = 1.f;
        greenWidget.Style.Margin = Edges{ 20.f };
        rootWidget.AddChild( greenWidget );

        Widget blueWidget;
        blueWidget.Style.FixedHeight = 120.f;
        blueWidget.Style.WidthMode = ESizingMode::Fill;
		blueWidget.Style.HeightMode = ESizingMode::Fill;
        blueWidget.Style.PercentWidth = 1.f;
        rootWidget.AddChild( blueWidget );

        // Measure and arrange the widget tree to compute final positions and sizes
		i32 windowWidth, windowHeight;
		SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );

        const Vec2f availableSize{ static_cast<f32>( windowWidth ), static_cast<f32>( windowHeight ) };
        MeasureWidget( rootWidget, availableSize );
        const Rectf rootRect{ .Origin = Vec2f{ 0.f, 0.f }, .Size = availableSize };
        ArrangeWidget( rootWidget, rootRect );

        // Render the widgets using the SDL2Renderer
        SDL_Renderer* sdl = GetRenderer();
        
        SDL_SetRenderDrawColor( sdl, 255, 255, 255, 255 );
        SDL_Rect d{ 0,0, windowWidth, windowHeight };
        SDL_RenderFillRect( sdl, &d );

		SDL_SetRenderDrawColor( sdl, 255, 0, 0, 255 );
        SDL_Rect redRect{ static_cast<i32>( redWidget.Layout.FinalRect.Origin[ 0 ] ), static_cast<i32>( redWidget.Layout.FinalRect.Origin[ 1 ] ),
					  static_cast<i32>( redWidget.Layout.FinalRect.Size[0] ), static_cast<i32>( redWidget.Layout.FinalRect.Size[1] ) };
		SDL_RenderFillRect( sdl, &redRect );

		SDL_SetRenderDrawColor( sdl, 0, 255, 0, 255 );
		SDL_Rect greenRect{ static_cast<i32>( greenWidget.Layout.FinalRect.Origin[0] ), static_cast<i32>( greenWidget.Layout.FinalRect.Origin[1] ),
                        static_cast<i32>( greenWidget.Layout.FinalRect.Size[0] ), static_cast<i32>( greenWidget.Layout.FinalRect.Size[1] ) };
		SDL_RenderFillRect( sdl, &greenRect );

		SDL_SetRenderDrawColor( sdl, 0, 0, 255, 255 );
		SDL_Rect blueRect{ static_cast<i32>( blueWidget.Layout.FinalRect.Origin[0] ), static_cast<i32>( blueWidget.Layout.FinalRect.Origin[1] ),
                       static_cast<i32>( blueWidget.Layout.FinalRect.Size[0] ), static_cast<i32>( blueWidget.Layout.FinalRect.Size[1] ) };
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
