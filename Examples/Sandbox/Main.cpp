#include <Application.h>
#include "RatUI/RatUI.h"
#include <RatUI/Renderer/DrawList.h>
#include <RatUI/Input/InputEvent.h>
#include <RatUI/Input/Navigation.h>
#include <RatUI/Widget/TextWidget.h>
#include <print>
#include <format>
#include <iostream>
#include <functional>

using namespace RatUI;

static SDL_Renderer* g_SDLRenderer = nullptr;

class TextMetrics : public ITextMetrics
{
public:

    TextMeasurement Measure( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth ) override
    {
        const f32 charWidth = a_Style.Size * 0.5f;
		const f32 advance = charWidth + a_Style.LetterSpacing;

        f32 lineWidth = 0.f;
        f32 maxLineWidth = 0.f;
        u32 lineCount = 1;

        for ( char c : a_Text )
        {
            if ( c == '\n' )
            {
                lineCount++;
                maxLineWidth = std::max( maxLineWidth, lineWidth );
                lineWidth = 0.f;
            }
            else
            {
                // Check wrap BEFORE adding this character, same as the renderer
                if ( lineWidth > 0.f && lineWidth + charWidth > a_MaxWidth )
                {
                    lineCount++;
                    maxLineWidth = std::max( maxLineWidth, lineWidth );
                    lineWidth = 0.f;
                }
                lineWidth += advance;
            }
        }

        // Remove trailing spacing from last character on each line
        if ( lineWidth > 0.f )
			lineWidth -= a_Style.LetterSpacing;

        maxLineWidth = std::max( maxLineWidth, lineWidth );

        return TextMeasurement{ .Size = { maxLineWidth, static_cast<f32>( lineCount ) * a_Style.Size } };
    }

    virtual ShapedText Shape( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() )
    {
        return ShapedText{};
    }

    void ReleaseShapedText( const ShapedText& a_ShapedText ) override
    {
    }
};

EButtonID SDLKeyboardToButtonID( SDL_Keycode a_Keycode )
{
    // Map SDL keycodes to our EButtonID enum. This is a simplified mapping for demonstration purposes.
    if ( a_Keycode >= SDLK_a && a_Keycode <= SDLK_z )
        return static_cast<EButtonID>( (int)EButtonID::KeyA + ( a_Keycode - SDLK_a ) );
    if ( a_Keycode >= SDLK_0 && a_Keycode <= SDLK_9 )
        return static_cast<EButtonID>( (int)EButtonID::Key0 + ( a_Keycode - SDLK_0 ) );
    switch ( a_Keycode )
    {
        case SDLK_RETURN: return EButtonID::KeyEnter;
        case SDLK_ESCAPE: return EButtonID::KeyEscape;
        case SDLK_SPACE:  return EButtonID::KeySpace;
        case SDLK_TAB:    return EButtonID::KeyTab;
        case SDLK_BACKSPACE: return EButtonID::KeyBackspace;
        case SDLK_UP:     return EButtonID::KeyUp;
        case SDLK_DOWN:   return EButtonID::KeyDown;
        case SDLK_LEFT:   return EButtonID::KeyLeft;
        case SDLK_RIGHT:  return EButtonID::KeyRight;
        default:          return EButtonID::Unknown; // Add more mappings as needed
    }
}

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
	bool IsHovered = false;
	bool IsPressed = false;
	bool IsPressable = true;

	void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& child )
        {
            IsPressable = false;
        } );

		time += 1.f / 60.f;

        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;

        // rotate when hovered
		f32 scale = !IsHovered ? 1.0f : 1.f + 0.05f * std::sin( time * 5.f ); // Scale oscillates between 1.0 and 1.1 when hovered

		if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddRect( SolidBrush{ .Color = Colors::White }, rect.Expanded( 10.f * scale ) );

		a_DrawList.AddRect( SolidBrush{ .Color = IsPressed ? Colors::White : Col }, rect.Expanded( 10 * 10 * ( scale - 1.f ) ) );

        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
	}

	void OnPointerEnter( Scene& a_Scene, const PointerEvent& a_Event ) override
    {
        if ( !IsPressable )
			return;

		IsHovered = true;
    }

	void OnPointerExit( Scene& a_Scene, const PointerEvent& a_Event ) override
	{
        if ( !IsPressable )
            return;

		IsHovered = false;
	}

    bool OnPressed( Scene& a_Scene, const ButtonEvent& a_Event ) override
    {
        if ( !IsPressable )
            return false;

		IsPressed = true;
        return true;
	}

    bool OnReleased( Scene& a_Scene, const ButtonEvent& a_Event ) override
    {
        if ( !IsPressable )
            return false;

		IsPressed = false;
		return true;
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
    TextMetrics m_TextMetrics;
    WidgetID green;

    bool OnInitialize() override
    {
        g_SDLRenderer = GetRenderer();
        m_Scene.TextMetrics = &m_TextMetrics;
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

        // ---------------- TEXT ----------------
		WidgetID text = m_Scene.CreateWidget<TextWidget>( blue, "Hello, RatUI!", TextStyle{ .Size = 32.f, .LetterSpacing = 2.f } );
        auto* textNode = m_Scene.Layouts.Get( m_Scene.GetWidget( text )->GetLayoutID() );
        textNode->Style.WidthMode = ESizingMode::Content;
        textNode->Style.HeightMode = ESizingMode::Content;

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

        // Clear
        SDL_SetRenderDrawColor( g_SDLRenderer, 255, 255, 255, 255 );
        SDL_RenderClear( g_SDLRenderer );

        // Layout
        m_Scene.UpdateLayout( size );

        // Process SDL events and dispatch them to the scene
        {
            SDL_Event sdlEvent;
            while ( SDL_PollEvent( &sdlEvent ) )
            {
                if ( sdlEvent.type == SDL_QUIT )
                {
                    RequestExit();
                    return;
                }
                else if ( sdlEvent.type == SDL_KEYDOWN || sdlEvent.type == SDL_KEYUP )
                {
                    InputEvent event{
                        .Device = EDeviceID::Keyboard,
                        .Payload = ButtonEvent{
							.Button = SDLKeyboardToButtonID( sdlEvent.key.keysym.sym ),
                            .Pressed = sdlEvent.type == SDL_KEYDOWN,
                            .Released = sdlEvent.type == SDL_KEYUP,
                            .Held = false // Held state can be tracked separately if needed
                        }
                    };

                    m_Scene.DispatchInputEvent( event );

                    // TEst out navigation with arrow keys
                    if ( event.Device == EDeviceID::Keyboard )
                    {
                        const ButtonEvent& btnEvent = std::get<ButtonEvent>( event.Payload );

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
                    }
                }
                else if ( sdlEvent.type == SDL_MOUSEBUTTONDOWN || sdlEvent.type == SDL_MOUSEBUTTONUP )
                {
                    InputEvent event{
                        .Device = EDeviceID::Mouse,
                        .Payload = ButtonEvent{
                            .Button = static_cast<EButtonID>( SDL_BUTTON( sdlEvent.button.button ) ),
                            .Pressed = sdlEvent.type == SDL_MOUSEBUTTONDOWN,
                            .Released = sdlEvent.type == SDL_MOUSEBUTTONUP,
                            .Held = false // Held state can be tracked separately if needed
                        }
                    };

                    m_Scene.DispatchInputEvent( event );
                }
                else if ( sdlEvent.type == SDL_MOUSEWHEEL )
                {
                    InputEvent event{
                        .Device = EDeviceID::Mouse,
                        .Payload = PointerEvent{
                            .Type = EPointerType::Mouse,
                            .ScrollDelta = Vec2f{ (f32)sdlEvent.wheel.x, (f32)sdlEvent.wheel.y }
                        }
                    };

                    m_Scene.DispatchInputEvent( event );
                }
                else if ( sdlEvent.type == SDL_MOUSEMOTION )
                {
                    InputEvent event{
                        .Device = EDeviceID::Mouse,
                        .Payload = PointerEvent{
                            .Position = Vec2f{ (f32)sdlEvent.motion.x, (f32)sdlEvent.motion.y },
                            .Delta = Vec2f{ (f32)sdlEvent.motion.xrel, (f32)sdlEvent.motion.yrel },
                            .Type = EPointerType::Mouse
                        }
                    };

                    m_Scene.DispatchInputEvent( event );
                }
            }
        }
    }

    void OnRender() override
    {
        using namespace RatUI;

		DrawList drawList;
        m_Scene.Render( drawList );

        for ( const DrawCmd& cmd : drawList.Commands )
        {
			SDL_Color sdlColor = { 255, 255, 255, 255 }; // Default to white if no brush color is specified
            if ( Holds<SolidBrush>(cmd.DrawBrush) )
            {
                const auto& solid = std::get<SolidBrush>( cmd.DrawBrush );
                sdlColor = {
                    static_cast<Uint8>( solid.Color[0] * 255.f ),
                    static_cast<Uint8>( solid.Color[1] * 255.f ),
                    static_cast<Uint8>( solid.Color[2] * 255.f ),
                    static_cast<Uint8>( solid.Color[3] * 255.f )
                };
                SDL_SetRenderDrawColor( g_SDLRenderer, sdlColor.r, sdlColor.g, sdlColor.b, sdlColor.a );
			}

            if ( std::holds_alternative<DrawCmd::RectCmd>( cmd.Payload ) )
            {
                const auto& rectCmd = std::get<DrawCmd::RectCmd>( cmd.Payload );
                const Rectf& rect = rectCmd.Rect;
                
                const Mat3f& transform = cmd.Transform;

                // Apply transform to rect corners
                Vec3f topLeft = transform * Vec3f{ rect.Left(), rect.Top(), 1.f };
                Vec3f topRight = transform * Vec3f{ rect.Right(), rect.Top(), 1.f };
                Vec3f bottomLeft = transform * Vec3f{ rect.Left(), rect.Bottom(), 1.f };
                Vec3f bottomRight = transform * Vec3f{ rect.Right(), rect.Bottom(), 1.f };

                SDL_Vertex vertices[4] = {
                    {.position = { topLeft[0], topLeft[1] }, .color = sdlColor, .tex_coord = {0.f, 0.f}},
                    {.position = { topRight[0], topRight[1] }, .color = sdlColor, .tex_coord = { 1.f, 0.f } },
                    {.position = { bottomRight[0], bottomRight[1] }, .color = sdlColor, .tex_coord = { 1.f, 1.f } },
                    {.position = { bottomLeft[0], bottomLeft[1] }, .color = sdlColor, .tex_coord = { 0.f, 1.f } }
                };

                int indices[6] = { 0, 1, 2, 0, 2, 3 };

                // Draw filled rect (as two triangles)
                SDL_RenderGeometry( g_SDLRenderer, nullptr,
                    vertices, 4,
                    indices, 6 );
            }
            else if ( Holds<DrawCmd::TextCmd>( cmd.Payload ) )
            {
                const auto& textCmd = std::get<DrawCmd::TextCmd>( cmd.Payload );
                const size_t count = textCmd.Text.size();
                if ( count == 0 ) continue;

                const Mat3f& transform = cmd.Transform;
                const Rectf& rect = textCmd.Rect;
                const float  charWidth = textCmd.Style.Size * 0.5f;
                const float  charHeight = textCmd.Style.Size;
				const float  spacing = textCmd.Style.LetterSpacing;

                float cursorX = rect.Left();
                float cursorY = rect.Top();

                for ( size_t i = 0; i < count; ++i )
                {
                    const char c = textCmd.Text[i];

                    // Wrap if we'd exceed the rect right edge (and we're not at line start)
                    if ( c == '\n' || ( cursorX > rect.Left() && cursorX + charWidth > rect.Right() ) )
                    {
                        cursorX = rect.Left();
                        cursorY += charHeight;
                    }

                    if ( c == '\n' ) continue;

                    Vec3f tl = transform * Vec3f{ cursorX,             cursorY,              1.f };
                    Vec3f tr = transform * Vec3f{ cursorX + charWidth, cursorY,              1.f };
                    Vec3f bl = transform * Vec3f{ cursorX,             cursorY + charHeight, 1.f };
                    Vec3f br = transform * Vec3f{ cursorX + charWidth, cursorY + charHeight, 1.f };

                    SDL_Vertex vertices[4] = {
                        { { tl[0], tl[1] }, sdlColor, { 0.f, 0.f } },
                        { { tr[0], tr[1] }, sdlColor, { 1.f, 0.f } },
                        { { br[0], br[1] }, sdlColor, { 1.f, 1.f } },
                        { { bl[0], bl[1] }, sdlColor, { 0.f, 1.f } }
                    };
                    int indices[6] = { 0, 1, 2, 0, 2, 3 };
                    SDL_RenderGeometry( g_SDLRenderer, nullptr, vertices, 4, indices, 6 );

                    cursorX += charWidth + spacing;
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
