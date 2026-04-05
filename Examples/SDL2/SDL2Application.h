#pragma once
#include <RatUI/RatUI.h>
#include <RatUI/Backends/SDL2/SDL2Renderer.h>
#include <SDL2/SDL.h>
#include <iostream>

using namespace RatUI;

static EButtonID SDLKeyboardToRatUIButtonID( SDL_Keycode a_Keycode )
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

/**
 * @brief
 * Provides SDL2-backed windowing, event processing and a render loop.
 * Derive from this class and override the four virtual hooks to implement
 * your example.
 */
class SDL2Application
{
public:
    /**
     * @brief Configuration parameters for the application window.
     */
    struct Config
    {
        std::string Title  = "RatUI Application";
        Colorf      ClearColor{ 0.f, 0.f, 0.f, 1.f };
        int         Width  = 1280;
        int         Height = 720;
    };

    /** @brief Constructs an application with the window configuration. */
    SDL2Application( Config a_Config = {} ) : m_Config( a_Config ) {}
    virtual ~SDL2Application() = default;

    /** @brief Returns the underlying SDL_Window handle. */
    SDL_Window*   GetWindow()   const { return m_Window; }

    /** @brief Returns the underlying SDL_Renderer handle. */
    SDL_Renderer* GetRenderer() const { return m_SDLRenderer; }

    /** @brief Requests the application to exit the main loop and shut down. */
    void RequestExit() { m_Running = false; }

    /** @brief Checks if the application is still running. */
    bool IsRunning() const { return m_Running; }

    bool Run()
    {
        if ( !Initialize() )
            return false;

        try
        {
            while ( IsRunning() )
            {
                ProcessEvents();
                OnUpdate();

                if ( m_SDLRenderer )
                {
                    SDL_SetRenderDrawColor( m_SDLRenderer, m_Config.ClearColor[0] * 255, m_Config.ClearColor[1] * 255, m_Config.ClearColor[2] * 255, m_Config.ClearColor[3] * 255 );
                    SDL_RenderClear( m_SDLRenderer );
                    OnRender( m_Renderer );
                    SDL_RenderPresent( m_SDLRenderer );
                }
            }
        }
        catch ( const std::exception& )
        {
            Shutdown();
            return false;
        }
        catch ( ... )
        {
            Shutdown();
            return false;
        }

        return Shutdown();
    }

protected:
    virtual bool OnInitialize() { return true; };
    virtual void OnUpdate() {};
    virtual void OnRender( IRenderer& a_Renderer ) {};
    virtual bool OnShutdown() { return true; };
	virtual void OnInputEvent( const RatUI::InputEvent& ) {}

private:
    bool Initialize()
    {
        if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
            return false;

        m_Window = SDL_CreateWindow(
            m_Config.Title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            m_Config.Width,
            m_Config.Height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if ( !m_Window )
        {
            SDL_Quit();
            return false;
        }

        m_SDLRenderer = SDL_CreateRenderer(
            m_Window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if ( !m_SDLRenderer )
        {
            SDL_DestroyWindow( m_Window );
            m_Window = nullptr;
            SDL_Quit();
            return false;
        }

        m_Renderer.SetSDLRenderer( m_SDLRenderer );

        return OnInitialize();
    }

    void ProcessEvents()
    {
        SDL_Event sdlEvent;
        while ( SDL_PollEvent( &sdlEvent ) )
        {
            InputEvent inputEvent{};

            if ( sdlEvent.type == SDL_QUIT )
            {
                RequestExit();
                return;
            }
            else if ( sdlEvent.type == SDL_KEYDOWN || sdlEvent.type == SDL_KEYUP )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Keyboard,
                    .Payload = ButtonEvent{
						.Button = SDLKeyboardToRatUIButtonID( sdlEvent.key.keysym.sym ),
                        .Pressed = sdlEvent.type == SDL_KEYDOWN,
                        .Released = sdlEvent.type == SDL_KEYUP,
                        .Held = false // Held state can be tracked separately if needed
                    }
                };
            }
            else if ( sdlEvent.type == SDL_MOUSEBUTTONDOWN || sdlEvent.type == SDL_MOUSEBUTTONUP )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Mouse,
                    .Payload = ButtonEvent{
                        .Button = static_cast<EButtonID>( SDL_BUTTON( sdlEvent.button.button ) ),
                        .Pressed = sdlEvent.type == SDL_MOUSEBUTTONDOWN,
                        .Released = sdlEvent.type == SDL_MOUSEBUTTONUP,
                        .Held = false // Held state can be tracked separately if needed
                    }
                };
            }
            else if ( sdlEvent.type == SDL_MOUSEWHEEL )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Mouse,
                    .Payload = PointerEvent{
                        .Type = EPointerType::Mouse,
                        .ScrollDelta = Vec2f{ (f32)sdlEvent.wheel.x, (f32)sdlEvent.wheel.y }
                    }
                };
            }
            else if ( sdlEvent.type == SDL_MOUSEMOTION )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Mouse,
                    .Payload = PointerEvent{
                        .Position = Vec2f{ (f32)sdlEvent.motion.x, (f32)sdlEvent.motion.y },
                        .Delta = Vec2f{ (f32)sdlEvent.motion.xrel, (f32)sdlEvent.motion.yrel },
                        .Type = EPointerType::Mouse
                    }
                };
            }

			OnInputEvent( inputEvent );
        }
    }

    bool Shutdown()
    {
        bool result = OnShutdown();

        m_Renderer = {};

        if ( m_SDLRenderer )
        {
            SDL_DestroyRenderer( m_SDLRenderer );
            m_SDLRenderer = nullptr;
        }

        if ( m_Window )
        {
            SDL_DestroyWindow( m_Window );
            m_Window = nullptr;
        }

        SDL_Quit();
        return result;
    }

protected:
    Config              m_Config;
    bool                m_Running{ true };
    SDL_Window*         m_Window{ nullptr };
    SDL_Renderer*       m_SDLRenderer{ nullptr };
    SDL2::SDL2Renderer  m_Renderer{};
};