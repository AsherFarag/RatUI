#include "SDL2Application.h"
#include <print>
#include <format>
#include <iostream>
#include <functional>

using namespace RatUI;
using namespace RatUI::Literals;

class RectWidget : public IWidget
{
public:
	RectWidget( Colorf a_Color, StringView a_Name, CornerRounding a_Rounding = CornerRounding::Uniform( 10_deg ) )
		: Color( a_Color )
		, Name( a_Name )
        , Rounding( a_Rounding )
    {}

	StringView Name;
    Colorf Color;
    CornerRounding Rounding;
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

		//a_DrawList.PushTransform( transform.ToMatrix( rect ) );

		if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddRect( Colors::White, rect.Expanded( 4.f ), Rounding + 4_deg );

		a_DrawList.AddRect( Color, rect, Rounding );

        a_DrawList.PushClipRect( rect );
        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );
        a_DrawList.PopClipRect();

        //a_DrawList.PopTransform();
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
    Colorf Color;
    bool IsFilled;

    CircleWidget( f32 a_Radius, Colorf a_Color, bool a_Filled = true )
        : Radius( a_Radius )
        , Color( a_Color )
        , IsFilled( a_Filled )
    {}

    void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;
        Vec2f center = rect.Center();

        if ( IsFilled )
        {
            if ( a_Scene.GetFocusedWidget() == GetID() )
			    a_DrawList.AddCircle( Colors::LightYellow, center, Radius + 4.f );

            a_DrawList.AddCircle( Color, center, Radius );
        }
        else
        {
            const f32 borderThickness = 4.f;
            if ( a_Scene.GetFocusedWidget() == GetID() )
                a_DrawList.AddCircleBorder( Colors::LightYellow, center, Radius + 4.f, borderThickness + 2.f );

            a_DrawList.AddCircleBorder( Color, center, Radius, borderThickness );
        }
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
        : SDL2Application( { "RatUI Sandbox", Colors::Surface900, 1280, 720 } )
    {}

protected:

    Scene m_Scene;
    WidgetID green;

bool OnInitialize() override
{
    // Root container - deep app background
    WidgetID trueRoot = m_Scene.CreateRootWidget<RectWidget>( Colors::Surface900, "AppBackground" );
    LayoutNode* trueRootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( trueRoot )->GetLayoutID() );
    trueRootNode->Style.LayoutType = ELayoutType::Vertical;
    trueRootNode->Style.Spacing = 10.f;
    trueRootNode->Style.Padding = Edges{ 10.f };
    trueRootNode->Style.WidthMode = ESizingMode::Flex;
    trueRootNode->Style.HeightMode = ESizingMode::Flex;

    // Main panel - slightly lighter surface
    WidgetID root = m_Scene.CreateWidget<RectWidget>( trueRoot, Colors::Surface800, "MainPanel" );
    LayoutNode* rootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( root )->GetLayoutID() );
    rootNode->Style.LayoutType = ELayoutType::Vertical;
    rootNode->Style.Spacing = 10.f;
    rootNode->Style.Padding = Edges{ 10.f };
    rootNode->Style.WidthMode = ESizingMode::Flex;
    rootNode->Style.HeightMode = ESizingMode::Flex;
    rootNode->Style.IsFocusScope = true;

    // ---------------- HEADER BAR (was Red) ----------------
    WidgetID headerBar = m_Scene.CreateWidget<RectWidget>( root, Colors::AccentBlue, "HeaderBar" );
    auto* headerBarNode = m_Scene.Layouts.Get( m_Scene.GetWidget( headerBar )->GetLayoutID() );

    headerBarNode->Style.FixedHeight = 100.f;
    headerBarNode->Style.WidthMode = ESizingMode::Flex;
    headerBarNode->Style.HeightMode = ESizingMode::Fixed;
    headerBarNode->Style.Margin = Edges{ 10.f };

    // ---------------- CONTENT ROW (was HBox) ----------------
    WidgetID contentRow = m_Scene.CreateWidget<RectWidget>( root, Colors::Surface700, "ContentRow" );
    auto* contentRowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( contentRow )->GetLayoutID() );

    contentRowNode->Style.LayoutType = ELayoutType::Horizontal;
    contentRowNode->Style.Spacing = 0.f;
    contentRowNode->Style.Padding = Edges{ 10.f };
    contentRowNode->Style.HeightMode = ESizingMode::Fixed;
    contentRowNode->Style.FixedHeight = 150.f;
    contentRowNode->Style.WidthMode = ESizingMode::Flex;
    contentRowNode->Style.Spacing = 20.f;
    contentRowNode->Style.IsFocusScope = true;

    // ---------------- MAIN CONTENT AREA ----------------
    green = m_Scene.CreateWidget<RectWidget>( contentRow, Colors::Surface600, "MainContentArea" );
    auto* mainContentNode = m_Scene.Layouts.Get( m_Scene.GetWidget( green )->GetLayoutID() );

    mainContentNode->Style.WidthMode = ESizingMode::Flex;
    mainContentNode->Style.HeightMode = ESizingMode::Flex;
    mainContentNode->Style.FlexGrow = 1.f;
    mainContentNode->Style.PercentWidth = 0.5f;
    mainContentNode->Style.Margin = Edges{ 10.f };

    // ---------------- SIDEBAR PANEL ----------------
    {
        WidgetID sidebarPanel = m_Scene.CreateWidget<RectWidget>( contentRow, Colors::Surface600, "SidebarPanel" );
        auto* sidebarNode = m_Scene.Layouts.Get( m_Scene.GetWidget( sidebarPanel )->GetLayoutID() );

        sidebarNode->Style.LayoutType = ELayoutType::Vertical;
        sidebarNode->Style.Spacing = 10.f;
        sidebarNode->Style.Padding = Edges{ 10.f };
        sidebarNode->Style.HeightMode = ESizingMode::Flex;
        sidebarNode->Style.WidthMode = ESizingMode::Fixed;
        sidebarNode->Style.FixedWidth = 100.f;
        sidebarNode->Style.ChildAlign = EAlignment::Center;
        sidebarNode->Style.IsFocusScope = true;

        // ---------------- STATUS INDICATOR ----------------
        WidgetID statusIndicator = m_Scene.CreateWidget<RectWidget>( sidebarPanel, Colors::AccentEmerald, "StatusIndicator" );
        auto* statusNode = m_Scene.Layouts.Get( m_Scene.GetWidget( statusIndicator )->GetLayoutID() );

        statusNode->Style.WidthMode = ESizingMode::Flex;
        statusNode->Style.FixedWidth = 40.f;
        statusNode->Style.HeightMode = ESizingMode::Flex;
        statusNode->Style.FixedHeight = 60.f;

        // ---------------- NOTIFICATION DOT ----------------
        WidgetID notificationDot = m_Scene.CreateWidget<RectWidget>( sidebarPanel, Colors::AccentRose, "NotificationDot" );
        auto* notifNode = m_Scene.Layouts.Get( m_Scene.GetWidget( notificationDot )->GetLayoutID() );

        notifNode->Style.WidthMode = ESizingMode::Flex;
        notifNode->Style.FixedWidth = 20.f;
        notifNode->Style.HeightMode = ESizingMode::Flex;
        notifNode->Style.FixedHeight = 20.f;
        notifNode->Style.Margin = Edges{ 10.f };
    }

    // ---------------- SECONDARY CONTENT AREA ----------------
    WidgetID secondaryContent = m_Scene.CreateWidget<RectWidget>( contentRow, Colors::Surface600, "SecondaryContentArea" );
    auto* secondaryNode = m_Scene.Layouts.Get( m_Scene.GetWidget( secondaryContent )->GetLayoutID() );

    secondaryNode->Style.WidthMode = ESizingMode::Flex;
    secondaryNode->Style.HeightMode = ESizingMode::Flex;
    secondaryNode->Style.PercentWidth = 0.5f;
    secondaryNode->Style.FlexGrow = 1.f;

    // ---------------- FOOTER BAR ----------------
    WidgetID footerBar = m_Scene.CreateWidget<RectWidget>( root, Colors::Surface700, "FooterBar" );
    auto* footerNode = m_Scene.Layouts.Get( m_Scene.GetWidget( footerBar )->GetLayoutID() );

    footerNode->Style.FixedHeight = 120.f;
    footerNode->Style.WidthMode = ESizingMode::Flex;
    footerNode->Style.HeightMode = ESizingMode::Fixed;

    // ---------------- ACCENT SWATCH ROW ----------------
    WidgetID accentSwatchRow = m_Scene.CreateWidget<RectWidget>( root, Colors::Surface800, "AccentSwatchRow" );
    auto* swatchRowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( accentSwatchRow )->GetLayoutID() );
    swatchRowNode->Style.LayoutType = ELayoutType::Horizontal;
    swatchRowNode->Style.Spacing = 20.f;
    swatchRowNode->Style.Padding = Edges{ 10.f };
    swatchRowNode->Style.HeightMode = ESizingMode::Fixed;
    swatchRowNode->Style.FixedHeight = 200.f;
    swatchRowNode->Style.WidthMode = ESizingMode::Flex;
    swatchRowNode->Style.IsFocusScope = true;

    // Five accent swatches: blue, purple, violet, emerald, rose
    constexpr Colorf accentSwatches[5] = {
        Colors::AccentBlue,
        Colors::AccentPurple,
        Colors::AccentViolet,
        Colors::AccentEmerald,
        Colors::AccentRose,
    };

    // Draw colored circles with increasing radius for each accent color
    for ( int i = 0; i < 5; ++i )
    {
        f32 radius = 30.f + i * 10.f;
        WidgetID swatch = m_Scene.CreateWidget<CircleWidget>( accentSwatchRow, radius, accentSwatches[i] );
        auto* swatchNode = m_Scene.Layouts.Get( m_Scene.GetWidget( swatch )->GetLayoutID() );
        swatchNode->Style.WidthMode = ESizingMode::Fixed;
        swatchNode->Style.FixedWidth = radius * 2.f;
        swatchNode->Style.HeightMode = ESizingMode::Fixed;
        swatchNode->Style.FixedHeight = radius * 2.f;
    }

    // Draw colored circle borders in reverse order on top of the swatches to create a layered effect
    for ( int i = 3; i >= 0; --i )
    {
        f32 radius = 30.f + i * 10.f;
        WidgetID swatch = m_Scene.CreateWidget<CircleWidget>( accentSwatchRow, radius, accentSwatches[i], false  );
        auto* swatchNode = m_Scene.Layouts.Get( m_Scene.GetWidget( swatch )->GetLayoutID() );
        swatchNode->Style.WidthMode = ESizingMode::Fixed;
        swatchNode->Style.FixedWidth = radius * 2.f;
        swatchNode->Style.HeightMode = ESizingMode::Fixed;
        swatchNode->Style.FixedHeight = radius * 2.f;
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
