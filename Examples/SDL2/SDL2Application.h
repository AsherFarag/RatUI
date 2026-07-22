#pragma once
#include <RatUI/RatUI.h>
#include <GL/glew.h>
#include <RatUI/Backends/OpenGL/OpenGLRenderer.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <memory>

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
 * @brief Configuration parameters for the application window.
 */
struct SDL2AppConfig
{
    std::string Title  = "RatUI Application";
    Vec4f       ClearColor{ 0.f, 0.f, 0.f, 1.f };
    int         Width  = 1280;
    int         Height = 720;
};

/**
 * @brief
 * Provides SDL2-backed windowing with an OpenGL 3.3 context, event processing
 * and a render loop.
 * Derive from this class and override the four virtual hooks to implement
 * your example.
 */
class SDL2Application
{
public:
    using Config = SDL2AppConfig;

    /** @brief Constructs an application with the window configuration. */
    SDL2Application( Config a_Config = {} ) : m_Config( std::move( a_Config ) ) {}
    virtual ~SDL2Application() = default;

    /** @brief Returns the underlying SDL_Window handle. */
    SDL_Window* GetWindow() const { return m_Window; }

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

                if ( m_Window && m_GLContext )
                {
                    glClearColor( m_Config.ClearColor[0], m_Config.ClearColor[1],
                                  m_Config.ClearColor[2], m_Config.ClearColor[3] );
                    glClear( GL_COLOR_BUFFER_BIT );

                    OnRender( *m_Renderer );

                    SDL_GL_SwapWindow( m_Window );
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

    /** @brief Returns the OpenGL renderer. */
    OpenGL::OpenGLRenderer* GetRenderer() const { return m_Renderer.get(); }

private:
    bool Initialize()
    {
        if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
            return false;

        // Request OpenGL 3.3 core profile.
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

        m_Window = SDL_CreateWindow(
            m_Config.Title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            m_Config.Width,
            m_Config.Height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if ( !m_Window )
        {
            SDL_Quit();
            return false;
        }

        m_GLContext = SDL_GL_CreateContext( m_Window );
        if ( !m_GLContext )
        {
            SDL_DestroyWindow( m_Window );
            m_Window = nullptr;
            SDL_Quit();
            return false;
        }

        SDL_GL_MakeCurrent( m_Window, m_GLContext );
        SDL_GL_SetSwapInterval( 1 ); // VSync

        // Load OpenGL function pointers via glad using SDL's loader.
        int gladLoaded = glewInit( );
        if ( gladLoaded )
        {
            SDL_GL_DeleteContext( m_GLContext );
            m_GLContext = nullptr;
            SDL_DestroyWindow( m_Window );
            m_Window = nullptr;
            SDL_Quit();
            return false;
        }

        m_Renderer = std::make_unique<OpenGL::OpenGLRenderer>( m_Config.Width, m_Config.Height );

        return OnInitialize();
    }

    void ProcessEvents()
    {
        const auto toMouseButton = +[](Uint8 a_SDLButton) -> EButtonID
        {
            switch (a_SDLButton)
            {
                case SDL_BUTTON_LEFT:   return EButtonID::MouseLeft;
                case SDL_BUTTON_RIGHT:  return EButtonID::MouseRight;
                case SDL_BUTTON_MIDDLE: return EButtonID::MouseMiddle;
                case SDL_BUTTON_X1:     return EButtonID::Mouse3;
                case SDL_BUTTON_X2:     return EButtonID::Mouse4;
                default:                return EButtonID::Unknown;
            }
        };

        const auto ToModifiers = +[](SDL_Keymod a_Mods) -> EModifier
        {
            EModifier result = EModifier::None;
        
            if (a_Mods & KMOD_LSHIFT) result |= EModifier::LShift;
            if (a_Mods & KMOD_RSHIFT) result |= EModifier::RShift;
            if (a_Mods & KMOD_LCTRL)  result |= EModifier::LCtrl;
            if (a_Mods & KMOD_RCTRL)  result |= EModifier::RCtrl;
            if (a_Mods & KMOD_LALT)   result |= EModifier::LAlt;
            if (a_Mods & KMOD_RALT)   result |= EModifier::RAlt;
            if (a_Mods & KMOD_LGUI)   result |= EModifier::LSuper;
            if (a_Mods & KMOD_RGUI)   result |= EModifier::RSuper;
        
            return result;
        };

		// TODO: Convert from Pixel space to Unit space for input events, and handle DPI scaling if needed.

        SDL_Event sdlEvent;
        while ( SDL_PollEvent( &sdlEvent ) )
        {
            InputEvent inputEvent{};

            if ( sdlEvent.type == SDL_QUIT )
            {
                RequestExit();
                return;
            }
            else if ( sdlEvent.type == SDL_WINDOWEVENT )
            {
                if ( sdlEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && m_Renderer )
                {
                    int w = sdlEvent.window.data1;
                    int h = sdlEvent.window.data2;
                    glViewport( 0, 0, w, h );
                    m_Renderer->SetViewport( w, h );
                }
            }
            else if ( sdlEvent.type == SDL_KEYDOWN || sdlEvent.type == SDL_KEYUP )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Keyboard,
                    .Payload = ButtonEvent{
						.Button = SDLKeyboardToRatUIButtonID( sdlEvent.key.keysym.sym ),
                        .Modifiers = ToModifiers(SDL_GetModState()),
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
                        .Button = toMouseButton( sdlEvent.button.button ),
                        .Modifiers = ToModifiers(SDL_GetModState()),
                        .Pressed = sdlEvent.type == SDL_MOUSEBUTTONDOWN,
                        .Released = sdlEvent.type == SDL_MOUSEBUTTONUP,
                        .Held = false, // Held state can be tracked separately if needed
						.Pointer = PointerID{ 0 }, // Assuming single mouse pointer with ID 0
						.PointerPosition = Vec2<Unit>{ Unit{ (f32)sdlEvent.button.x }, Unit{ (f32)sdlEvent.button.y } }
                    }
                };
            }
            else if ( sdlEvent.type == SDL_MOUSEWHEEL )
            {
                inputEvent = InputEvent{
                    .Device = EDeviceID::Mouse,
                    .Payload = PointerEvent{
                        .Type = EPointerType::Mouse,
                        .Modifiers = ToModifiers( SDL_GetModState() ),
                        .ScrollDelta = Vec2<Unit>{ Unit{ (f32)sdlEvent.wheel.x }, Unit{ (f32)sdlEvent.wheel.y } }
                    }
                };
            }
            else if ( sdlEvent.type == SDL_MOUSEMOTION )
            {
                inputEvent = InputEvent{
                    .Device       = EDeviceID::Mouse,
                    .Payload      = PointerEvent{
                        .Position = Vec2<Unit>{ Unit{ (f32)sdlEvent.motion.x }, Unit{ (f32)sdlEvent.motion.y } },
                        .Delta    = Vec2<Unit>{ Unit{ (f32)sdlEvent.motion.xrel }, Unit{ (f32)sdlEvent.motion.yrel } },
                        .Type     = EPointerType::Mouse,
                        .Modifiers = ToModifiers(SDL_GetModState()),
                    }
                };
            }

			OnInputEvent( inputEvent );
        }
    }

    bool Shutdown()
    {
        bool result = OnShutdown();

        m_Renderer.reset();

        if ( m_GLContext )
        {
            SDL_GL_DeleteContext( m_GLContext );
            m_GLContext = nullptr;
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
    Config                                     m_Config;
    bool                                       m_Running{ true };
    SDL_Window*                                m_Window{ nullptr };
    SDL_GLContext                              m_GLContext{ nullptr };
    std::unique_ptr<OpenGL::OpenGLRenderer>    m_Renderer;
};