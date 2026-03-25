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

        // ---------------- HBOX ----------------
        LayoutNode hbox;
        hbox.Style.LayoutType = ELayoutType::Horizontal;
        hbox.Style.Spacing = 10.f;
        hbox.Style.WidthMode = ESizingMode::Fill;
        hbox.Style.HeightMode = ESizingMode::Fixed;
        hbox.Style.FixedHeight = 150.f;
        hbox.Style.PercentWidth = 1.f;
        rootWidget.AddChild( hbox );

        // ---------------- GREEN ----------------
        LayoutNode greenWidget;
        greenWidget.Style.WidthMode = ESizingMode::Fill;
        greenWidget.Style.HeightMode = ESizingMode::Fill;
        greenWidget.Style.PercentWidth = 0.5f;
        hbox.AddChild( greenWidget );

        // ---------------- YELLOW ----------------
        LayoutNode yellowWidget;
        yellowWidget.Style.WidthMode = ESizingMode::Fill;
        yellowWidget.Style.HeightMode = ESizingMode::Fill;
        yellowWidget.Style.PercentWidth = 0.5f;
        hbox.AddChild( yellowWidget );

        // ---------------- BLUE ----------------
        LayoutNode blueWidget;
        blueWidget.Style.FixedHeight = 120.f;
        blueWidget.Style.WidthMode = ESizingMode::Fill;
        blueWidget.Style.HeightMode = ESizingMode::Fill;
        blueWidget.Style.PercentWidth = 1.f;
        rootWidget.AddChild( blueWidget );

        // ---------------- SCREEN ROOT (anchors need a full-screen parent) ----------------
        // Anchored widgets are positioned relative to their parent, so we need a full-screen
        // container to anchor against. This is separate from rootWidget.
        LayoutNode screenRoot;
        screenRoot.Style.LayoutType = ELayoutType::Overlay;
        screenRoot.Style.WidthMode = ESizingMode::Fill;
        screenRoot.Style.HeightMode = ESizingMode::Fill;
        screenRoot.Style.PercentWidth = 1.f;
        screenRoot.Style.PercentHeight = 1.f;

        // ---------------- TOP-LEFT ANCHOR ----------------
        LayoutNode topLeftWidget;
        topLeftWidget.Style.PositionMode = EPositioningMode::Anchored;
        topLeftWidget.Style.Anchor = Anchor::TopLeft();
        topLeftWidget.Style.WidthMode = ESizingMode::Fixed;
        topLeftWidget.Style.HeightMode = ESizingMode::Fixed;
        topLeftWidget.Style.FixedWidth = 80.f;
        topLeftWidget.Style.FixedHeight = 40.f;
        screenRoot.AddChild( topLeftWidget );

        // ---------------- TOP-RIGHT ANCHOR ----------------
        LayoutNode topRightWidget;
        topRightWidget.Style.PositionMode = EPositioningMode::Anchored;
        topRightWidget.Style.Anchor = Anchor::TopRight();
        topRightWidget.Style.WidthMode = ESizingMode::Fixed;
        topRightWidget.Style.HeightMode = ESizingMode::Fixed;
        topRightWidget.Style.FixedWidth = 80.f;
        topRightWidget.Style.FixedHeight = 40.f;
        screenRoot.AddChild( topRightWidget );

        // ---------------- BOTTOM-LEFT ANCHOR ----------------
        LayoutNode bottomLeftWidget;
        bottomLeftWidget.Style.PositionMode = EPositioningMode::Anchored;
        bottomLeftWidget.Style.Anchor = Anchor::BottomLeft();
        bottomLeftWidget.Style.WidthMode = ESizingMode::Fixed;
        bottomLeftWidget.Style.HeightMode = ESizingMode::Fixed;
        bottomLeftWidget.Style.FixedWidth = 80.f;
        bottomLeftWidget.Style.FixedHeight = 40.f;
        screenRoot.AddChild( bottomLeftWidget );

        // ---------------- BOTTOM-RIGHT ANCHOR ----------------
        LayoutNode bottomRightWidget;
        bottomRightWidget.Style.PositionMode = EPositioningMode::Anchored;
        bottomRightWidget.Style.Anchor = Anchor::BottomRight();
        bottomRightWidget.Style.WidthMode = ESizingMode::Fixed;
        bottomRightWidget.Style.HeightMode = ESizingMode::Fixed;
        bottomRightWidget.Style.FixedWidth = 80.f;
        bottomRightWidget.Style.FixedHeight = 40.f;
        screenRoot.AddChild( bottomRightWidget );

        // ---------------- CENTER ANCHOR (animated with offset) ----------------
        LayoutNode centerWidget;
        centerWidget.Style.PositionMode = EPositioningMode::Anchored;
        centerWidget.Style.Anchor = Anchor::Center();
        centerWidget.Style.WidthMode = ESizingMode::Fixed;
        centerWidget.Style.HeightMode = ESizingMode::Fixed;
        centerWidget.Style.FixedWidth = 120.f;
        centerWidget.Style.FixedHeight = 60.f;
        screenRoot.AddChild( centerWidget );

        // ---------------- STRETCH ANCHOR (full-width bottom bar) ----------------
        LayoutNode bottomBarWidget;
        bottomBarWidget.Style.PositionMode = EPositioningMode::Anchored;
        bottomBarWidget.Style.Anchor = Anchor::StretchBottom();
        bottomBarWidget.Style.HeightMode = ESizingMode::Fixed;
        bottomBarWidget.Style.FixedHeight = 50.f;
        screenRoot.AddChild( bottomBarWidget );

        // ---------------- ANIMATION ----------------
        f32 time = static_cast<f32>( SDL_GetTicks() ) / 1000.f;

        hbox.Style.FixedHeight = 50 + 150.f * ( 0.5f + 0.5f * std::sin( time ) );

        greenWidget.Style.FlexGrow = 0.5f + 0.5f * std::sin( time * 0.5f );
        yellowWidget.Style.FlexGrow = 2.f;

        // Animate center widget offset so it orbits the center point
        centerWidget.Style.Anchor.Offset = {
            80.f * std::cos( time ),
            80.f * std::sin( time )
        };

        // ---------------- LAYOUT PASS ----------------
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );

        const Vec2f availableSize{ (f32)windowWidth, (f32)windowHeight };

        // Layout the flow widgets
        MeasureLayoutNode( rootWidget, availableSize );
        ArrangeLayoutNode( rootWidget, { .Origin = { 0.f, 0.f }, .Size = availableSize } );

        // Layout the anchored screen overlay separately
        MeasureLayoutNode( screenRoot, availableSize );
        ArrangeLayoutNode( screenRoot, { .Origin = { 0.f, 0.f }, .Size = availableSize } );

        // ---------------- RENDER ----------------
        SDL_Renderer* sdl = GetRenderer();

        auto RenderNode = [&]( const LayoutNode& node, u8 r, u8 g, u8 b )
        {
            SDL_SetRenderDrawColor( sdl, r, g, b, 255 );
            SDL_Rect rect{
                (i32)node.Layout.FinalRect.Origin[0],
                (i32)node.Layout.FinalRect.Origin[1],
                (i32)node.Layout.FinalRect.Size[0],
                (i32)node.Layout.FinalRect.Size[1]
            };
            SDL_RenderFillRect( sdl, &rect );
        };

        // Clear
        SDL_SetRenderDrawColor( sdl, 255, 255, 255, 255 );
        SDL_Rect d{ 0, 0, windowWidth, windowHeight };
        SDL_RenderFillRect( sdl, &d );

        // Flow widgets
        RenderNode( redWidget, 255, 0, 0 );
        RenderNode( greenWidget, 0, 255, 0 );
        RenderNode( yellowWidget, 255, 255, 0 );
        RenderNode( blueWidget, 0, 0, 255 );

        // Anchored widgets - rendered on top of everything else
        RenderNode( topLeftWidget, 180, 0, 180 ); // purple
        RenderNode( topRightWidget, 180, 0, 180 );
        RenderNode( bottomBarWidget, 40, 40, 40 ); // dark bar, full width
        RenderNode( bottomLeftWidget, 180, 0, 180 );
        RenderNode( bottomRightWidget, 180, 0, 180 );
        RenderNode( centerWidget, 255, 140, 0 ); // orange, orbits center

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
