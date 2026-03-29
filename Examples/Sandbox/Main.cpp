#include <Application.h>
#include "RatUI/RatUI.h"
#include <RatUI/Renderer/DrawList.h>
#include <RatUI/Input/InputEvent.h>
#include <print>
#include <format>
#include <iostream>
#include <functional>

using namespace RatUI;

static SDL_Renderer* g_SDLRenderer = nullptr;

class RectWidget : public IWidget
{
public:
	RectWidget( const SDL_Color& a_Color, StringView a_Name )
		: Col( a_Color.r, a_Color.g, a_Color.b, a_Color.a ), Name( a_Name )
    {}

	StringView Name;
    Colorf Col;
	f32 time = 0.f;

	void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
		time += 1.f / 60.f;

        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;

		// rotate between -30 and 30 degrees based on time, using the center of the rect as the pivot
		f32 angle = std::sin( time ) * 1.f;

        RenderTransform transform{}; transform.Angle = Radians{ Degreesf{ angle } };
		transform.Scale = Vec2f{ 1.f + 0.5f * std::sin( time * 0.5f ), 1.f + 0.5f * std::sin( time * 0.5f ) } * 0.5f; // Scale between 0.5 and 1.5

		//a_DrawList.PushTransform( transform.ToMatrix( rect ) );
		a_DrawList.AddRect( SolidBrush{ .Color = Col }, rect );

        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );

        //a_DrawList.PopTransform();
    }

    void OnHoverEnter( Scene& a_Scene ) override
    {
		std::cout << "Hover Enter: " << Name << std::endl;
    }

    void OnHoverExit( Scene& a_Scene ) override
	{
		std::cout << "Hover Exit: " << Name << std::endl;
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
    WidgetID green;

    bool OnInitialize() override
    {
        g_SDLRenderer = GetRenderer();

        Brush brush = SolidBrush{ .Color = Colors::PowderBlue };
		DrawList testDrawList;
		testDrawList
			.AddRect( brush, Rectf::FromCenter( Vec2f{ 100.f, 100.f }, Vec2f{ 50.f, 50.f } ) )
			.AddRectBorder( brush, Rectf::FromCenter( Vec2f{ 200.f, 100.f }, Vec2f{ 50.f, 50.f } ), 5.f )
            .PushClipRect( Rectf::FromCenter( Vec2f{ 300.f, 100.f }, Vec2f{ 50.f, 50.f } ) )
                .AddRect( brush, Rectf::FromCenter( Vec2f{ 300.f, 100.f }, Vec2f{ 50.f, 50.f } ) )
                .AddCustom( brush, nullptr )
			.PopClipRect();

        // Root container
        WidgetID root = m_Scene.CreateRootWidget<RectWidget>( SDL_Color{ 0, 0, 0, 0 }, "Root" );
        m_Scene.RootWidget = root;

        LayoutNode* rootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( root )->GetLayoutID() );
        rootNode->Style.LayoutType = ELayoutType::Vertical;
        rootNode->Style.Spacing = 10.f;
        rootNode->Style.Padding = Edges{ 10.f };
        rootNode->Style.WidthMode = ESizingMode::Fill;
        rootNode->Style.HeightMode = ESizingMode::Fill;

        // ---------------- RED ----------------
        WidgetID red = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 255, 0, 0, 255 }, "Red" );
        auto* redNode = m_Scene.Layouts.Get( m_Scene.GetWidget( red )->GetLayoutID() );

        redNode->Style.FixedHeight = 100.f;
        redNode->Style.WidthMode = ESizingMode::Fill;
        redNode->Style.HeightMode = ESizingMode::Fixed;
        redNode->Style.Margin = Edges{ 10.f };

        // ---------------- HBOX ----------------
        WidgetID hbox = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 0, 0, 50, 0 }, "HBox" );
        auto* hboxNode = m_Scene.Layouts.Get( m_Scene.GetWidget( hbox )->GetLayoutID() );

        hboxNode->Style.LayoutType = ELayoutType::Horizontal;
        hboxNode->Style.Spacing = 0.f;
		hboxNode->Style.Padding = Edges{ 10.f };
        hboxNode->Style.HeightMode = ESizingMode::Fixed;
        hboxNode->Style.FixedHeight = 150.f;
        hboxNode->Style.WidthMode = ESizingMode::Fill;

        // ---------------- GREEN ----------------
        green = m_Scene.CreateWidget<RectWidget>( hbox, SDL_Color{ 0, 255, 0, 0 }, "Green" );
        auto* greenNode = m_Scene.Layouts.Get( m_Scene.GetWidget( green )->GetLayoutID() );

        greenNode->Style.WidthMode = ESizingMode::Fill;
        greenNode->Style.HeightMode = ESizingMode::Fill;
        greenNode->Style.FlexGrow = 1.f;
        greenNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.

        // ---------------- YELLOW ----------------
        WidgetID yellow = m_Scene.CreateWidget<RectWidget>( hbox, SDL_Color{ 255, 255, 0, 0 }, "Yellow" );
        auto* yellowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( yellow )->GetLayoutID() );

        yellowNode->Style.WidthMode = ESizingMode::Fill;
        yellowNode->Style.HeightMode = ESizingMode::Fill;
		yellowNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.
        yellowNode->Style.FlexGrow = 1.f;

        // ---------------- BLUE ----------------
        WidgetID blue = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 0, 0, 255, 255 }, "Blue" );
        auto* blueNode = m_Scene.Layouts.Get( m_Scene.GetWidget( blue )->GetLayoutID() );

        blueNode->Style.FixedHeight = 120.f;
        blueNode->Style.WidthMode = ESizingMode::Fill;
        blueNode->Style.HeightMode = ESizingMode::Fixed;

        return true;
    }

    void OnUpdate() override
    {
		f32 time = SDL_GetTicks() / 1000.f;

		// Animate green widget's width with a sine wave
        if ( auto* greenNode = m_Scene.Layouts.Get( m_Scene.GetWidget( green )->GetLayoutID() ) )
        {
            greenNode->Style.PercentWidth = 0.25f + 0.25f * std::sin( time );
            greenNode->Layout.IsDirty = true; // Mark layout dirty to trigger recalculation
		}

		// Get window size
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );

        const Vec2f size{ (f32)windowWidth, (f32)windowHeight };

        // Clear
        SDL_SetRenderDrawColor( g_SDLRenderer, 255, 255, 255, 255 );
        SDL_RenderClear( g_SDLRenderer );

        // Layout
        m_Scene.UpdateLayout( size );

        // Process input
        int mouseX, mouseY;
		bool mouseDown = SDL_GetMouseState( &mouseX, &mouseY ) & SDL_BUTTON( 1 );

		//m_Scene.ProcessInput( Vec2f{ (f32)mouseX, (f32)mouseY }, mouseDown, 1.f );
    }

    void OnRender() override
    {
        using namespace RatUI;

		DrawList drawList;
        m_Scene.Render( drawList );

        for ( const DrawCmd& cmd : drawList.Commands )
        {
            if ( std::holds_alternative<DrawCmd::RectCmd>( cmd.Payload ) )
            {
                const auto& rectCmd = std::get<DrawCmd::RectCmd>( cmd.Payload );
                const Rectf& rect = rectCmd.Rect;

                if ( std::holds_alternative<SolidBrush>( cmd.DrawBrush ) )
                {
                    const auto& solid = std::get<SolidBrush>( cmd.DrawBrush );
					SDL_SetRenderDrawColor( g_SDLRenderer, solid.Color[0], solid.Color[1], solid.Color[2], solid.Color[3] );

                    const Mat3f& transform = cmd.Transform;

                    // Apply transform to rect corners
                    Vec3f topLeft = transform * Vec3f{ rect.Left(), rect.Top(), 1.f };
                    Vec3f topRight = transform * Vec3f{ rect.Right(), rect.Top(), 1.f };
                    Vec3f bottomLeft = transform * Vec3f{ rect.Left(), rect.Bottom(), 1.f };
                    Vec3f bottomRight = transform * Vec3f{ rect.Right(), rect.Bottom(), 1.f };

                    // Draw filled rect (as two triangles)
                    SDL_RenderDrawLine( g_SDLRenderer, (int)topLeft[0], (int)topLeft[1], (int)topRight[0], (int)topRight[1] );
                    SDL_RenderDrawLine( g_SDLRenderer, (int)topRight[0], (int)topRight[1], (int)bottomRight[0], (int)bottomRight[1] );
                    SDL_RenderDrawLine( g_SDLRenderer, (int)bottomRight[0], (int)bottomRight[1], (int)bottomLeft[0], (int)bottomLeft[1] );
                    SDL_RenderDrawLine( g_SDLRenderer, (int)bottomLeft[0], (int)bottomLeft[1], (int)topLeft[0], (int)topLeft[1] );
                }
            }
		}
    }

    bool OnShutdown() override
    {
        return true;
    }


};

#undef main // SDL2 redefines main() on some platforms, so we undefine it here to avoid conflicts with our own main() function.

int main( int argc, char** argv )
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
