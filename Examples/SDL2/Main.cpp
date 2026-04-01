#include "SDL2Application.h"
#include <print>
#include <format>
#include <iostream>
#include <functional>

using namespace RatUI;

class RectWidget : public IWidget
{
public:
	RectWidget( const SDL_Color& a_Color, StringView a_Name )
		: Col( MakeColorU8( a_Color.r, a_Color.g, a_Color.b, a_Color.a ) )
		, Name( a_Name )
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
        transform.Scale = Vec2f{0.75,0.75} + Vec2f{ 1.f + 0.5f * std::sin( time * 0.5f ), 1.f + 0.5f * std::sin( time * 0.5f ) } * 0.1f; // Scale between 0.5 and 1.5

		a_DrawList.PushTransform( transform.ToMatrix( rect ) );

		constexpr CornerRounding rounding = CornerRounding{
            .TopLeft = Degreesf{ 10.f },
            .TopRight = Degreesf{ 30.f },
            .BottomLeft = Degreesf{ 50.f },
            .BottomRight = Degreesf{ 70.f }
        };

		if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddRect( Colors::White, rect.Expanded( 4.f ), rounding );
		using namespace RatUI::Literals;
		a_DrawList.AddRect( Col, rect, rounding );

        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );

        a_DrawList.PopTransform();
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
	}
};

class CircleWidget : public IWidget
{
public:

    f32 Radius;
    Colorf Col;

    CircleWidget( f32 a_Radius, const SDL_Color& a_Color )
        : Radius( a_Radius )
        , Col( MakeColorU8( a_Color.r, a_Color.g, a_Color.b, a_Color.a ) )
    {}

    void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;
        Vec2f center = rect.Center();

        if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddCircle( Colors::LightYellow, center, Radius + 4.f );

        a_DrawList.AddCircle( Col, center, Radius );
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
    }
};

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
        : SDL2Application( { "RatUI Sandbox", 1280, 720 } )
    {}

protected:

    Scene m_Scene;
    WidgetID green;

    bool OnInitialize() override
    {
        // Root container
        WidgetID trueRoot = m_Scene.CreateRootWidget<RectWidget>( SDL_Color{ 0, 0, 0, 0 }, "TrueRoot" );
        LayoutNode* trueRootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( trueRoot )->GetLayoutID() );
        trueRootNode->Style.LayoutType = ELayoutType::Vertical;
        trueRootNode->Style.Spacing = 10.f;
        trueRootNode->Style.Padding = Edges{ 10.f };
        trueRootNode->Style.WidthMode = ESizingMode::Flex;
        trueRootNode->Style.HeightMode = ESizingMode::Flex;

        WidgetID root = m_Scene.CreateWidget<RectWidget>( trueRoot, SDL_Color{ 50, 100, 150, 255 }, "Root" );

        LayoutNode* rootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( root )->GetLayoutID() );
        rootNode->Style.LayoutType = ELayoutType::Vertical;
        rootNode->Style.Spacing = 10.f;
        rootNode->Style.Padding = Edges{ 10.f };
        rootNode->Style.WidthMode = ESizingMode::Flex;
        rootNode->Style.HeightMode = ESizingMode::Flex;
		rootNode->Style.IsFocusScope = true; // Make the root a focus scope to test navigation between its children

        // ---------------- RED ----------------
        WidgetID red = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 255, 0, 0, 255 }, "Red" );
        auto* redNode = m_Scene.Layouts.Get( m_Scene.GetWidget( red )->GetLayoutID() );

        redNode->Style.FixedHeight = 100.f;
        redNode->Style.WidthMode = ESizingMode::Flex;
        redNode->Style.HeightMode = ESizingMode::Fixed;
        redNode->Style.Margin = Edges{ 10.f };

        // ---------------- HBOX ----------------
        WidgetID hbox = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 0, 0, 50, 255 }, "HBox" );
        auto* hboxNode = m_Scene.Layouts.Get( m_Scene.GetWidget( hbox )->GetLayoutID() );

        hboxNode->Style.LayoutType = ELayoutType::Horizontal;
        hboxNode->Style.Spacing = 0.f;
		hboxNode->Style.Padding = Edges{ 10.f };
        hboxNode->Style.HeightMode = ESizingMode::Fixed;
        hboxNode->Style.FixedHeight = 150.f;
        hboxNode->Style.WidthMode = ESizingMode::Flex;
		hboxNode->Style.IsFocusScope = true; // Make the HBox a focus scope to test navigation between its children

        // ---------------- GREEN ----------------
        green = m_Scene.CreateWidget<RectWidget>( hbox, SDL_Color{ 0, 255, 0, 255 }, "Green" );
        auto* greenNode = m_Scene.Layouts.Get( m_Scene.GetWidget( green )->GetLayoutID() );

        greenNode->Style.WidthMode = ESizingMode::Flex;
        greenNode->Style.HeightMode = ESizingMode::Flex;
        greenNode->Style.FlexGrow = 1.f;
        greenNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.

        // ---------------- YELLOW ----------------
        WidgetID yellow = m_Scene.CreateWidget<RectWidget>( hbox, SDL_Color{ 255, 255, 0, 255 }, "Yellow" );
        auto* yellowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( yellow )->GetLayoutID() );

        yellowNode->Style.WidthMode = ESizingMode::Flex;
        yellowNode->Style.HeightMode = ESizingMode::Flex;
		yellowNode->Style.PercentWidth = 0.5f; // This will be ignored since the parent is an HBox with Spacing, so it falls back to FlexGrow behavior.
        yellowNode->Style.FlexGrow = 1.f;
        
        // ---------------- BLUE ----------------
        WidgetID blue = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 0, 0, 255, 255 }, "Blue" );
        auto* blueNode = m_Scene.Layouts.Get( m_Scene.GetWidget( blue )->GetLayoutID() );

        blueNode->Style.FixedHeight = 120.f;
        blueNode->Style.WidthMode = ESizingMode::Flex;
        blueNode->Style.HeightMode = ESizingMode::Fixed;

        // ---------------- HBOX of CIRCLES ----------------
        WidgetID circleHBox = m_Scene.CreateWidget<RectWidget>( root, SDL_Color{ 50, 0, 50, 255 }, "CircleHBox" );
        auto* circleHBoxNode = m_Scene.Layouts.Get( m_Scene.GetWidget( circleHBox )->GetLayoutID() );
        circleHBoxNode->Style.LayoutType = ELayoutType::Horizontal;
        circleHBoxNode->Style.Spacing = 20.f;
        circleHBoxNode->Style.Padding = Edges{ 10.f };
        circleHBoxNode->Style.HeightMode = ESizingMode::Fixed;
        circleHBoxNode->Style.FixedHeight = 200.f;
        circleHBoxNode->Style.WidthMode = ESizingMode::Flex;
        circleHBoxNode->Style.IsFocusScope = true; // Make the HBox a focus scope to test navigation between its children

        for ( int i = 0; i < 5; ++i )
        {
            f32 radius = 30.f + i * 10.f;
            SDL_Color col = { (Uint8)( 255 - i * 40 ), (Uint8)( i * 40 ), 150, 255 };
            WidgetID circle = m_Scene.CreateWidget<CircleWidget>( circleHBox, radius, col );
            auto* circleNode = m_Scene.Layouts.Get( m_Scene.GetWidget( circle )->GetLayoutID() );
            circleNode->Style.WidthMode = ESizingMode::Fixed;
            circleNode->Style.FixedWidth = radius * 2.f;
            circleNode->Style.HeightMode = ESizingMode::Fixed;
            circleNode->Style.FixedHeight = radius * 2.f;
        }


        return true;
    }

    void OnUpdate() override
    {
		f32 time = SDL_GetTicks() / 1000.f;

		// Animate green widget's width with a sine wave
        if ( auto* greenNode = m_Scene.Layouts.Get( m_Scene.GetWidget( green )->GetLayoutID() ) )
        {
			greenNode->Style.FlexGrow = 0.5f + 0.5f * std::sin( time ); // FlexGrow oscillates between 0 and 1
            greenNode->Layout.IsDirty = true; // Mark layout dirty to trigger recalculation
		}

		// Get window size
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );
        const Vec2f size{ (f32)windowWidth, (f32)windowHeight };
        
        // Layout
        m_Scene.UpdateLayout( size );
    }

    void OnRender( IRenderer& a_Renderer ) override
    {
		DrawList drawList;
        m_Scene.Render( drawList );

        a_Renderer.Execute( drawList.Commands );
    }

    bool OnShutdown() override
    {
        return true;
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        // Test out navigation with arrow keys
        if ( a_Event.Device == EDeviceID::Keyboard )
        {
            const ButtonEvent& btnEvent = Get<ButtonEvent>( a_Event.Payload );

            ENavAction navAction = ENavAction::None;
            switch ( btnEvent.Button )
            {
                case EButtonID::KeyUp:    navAction = ENavAction::MoveUp; break;
                case EButtonID::KeyDown:  navAction = ENavAction::MoveDown; break;
                case EButtonID::KeyLeft:  navAction = ENavAction::MoveLeft; break;
                case EButtonID::KeyRight: navAction = ENavAction::MoveRight; break;
                case EButtonID::KeyEnter: navAction = ENavAction::Activate; break;
                case EButtonID::KeyEscape: navAction = ENavAction::Cancel; break;
                default: break; // Unsupported key
            }

            if ( btnEvent.Pressed && navAction != ENavAction::None )
            {
                m_Scene.Navigate( navAction );
            }

            return;
        }

        m_Scene.DispatchInputEvent( a_Event );
    }
};

#undef main // SDL2 redefines main() on some platforms, so we undefine it here to avoid conflicts with our own main() function.

int main( int argc, char** argv )
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
