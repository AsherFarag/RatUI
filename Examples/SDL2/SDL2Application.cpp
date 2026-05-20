#include "SDL2Application.h"

EButtonID SDL2Application::SDLKeyboardToRatUIButtonID( SDL_Keycode a_Keycode )
{
    if ( a_Keycode >= SDLK_a && a_Keycode <= SDLK_z )
        return static_cast<EButtonID>( (int)EButtonID::KeyA + ( a_Keycode - SDLK_a ) );
    if ( a_Keycode >= SDLK_0 && a_Keycode <= SDLK_9 )
        return static_cast<EButtonID>( (int)EButtonID::Key0 + ( a_Keycode - SDLK_0 ) );

    switch ( a_Keycode )
    {
        case SDLK_RETURN: return EButtonID::KeyEnter;
        case SDLK_ESCAPE: return EButtonID::KeyEscape;
        case SDLK_SPACE: return EButtonID::KeySpace;
        case SDLK_TAB: return EButtonID::KeyTab;
        case SDLK_BACKSPACE: return EButtonID::KeyBackspace;
        case SDLK_UP: return EButtonID::KeyUp;
        case SDLK_DOWN: return EButtonID::KeyDown;
        case SDLK_LEFT: return EButtonID::KeyLeft;
        case SDLK_RIGHT: return EButtonID::KeyRight;
        default: return EButtonID::Unknown;
    }
}

SDL2Application::SDL2Application( Config a_Config )
    : m_Config( std::move( a_Config ) )
{}

bool SDL2Application::Run()
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

bool SDL2Application::Initialize()
{
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
        return false;

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
    SDL_GL_SetSwapInterval( 1 );

    int gladLoaded = glewInit();
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

void SDL2Application::ProcessEvents()
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
                    .Pressed = sdlEvent.type == SDL_KEYDOWN,
                    .Released = sdlEvent.type == SDL_KEYUP,
                    .Held = false
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
                    .Held = false
                }
            };
        }
        else if ( sdlEvent.type == SDL_MOUSEWHEEL )
        {
            inputEvent = InputEvent{
                .Device = EDeviceID::Mouse,
                .Payload = PointerEvent{
                    .Type = EPointerType::Mouse,
                    .ScrollDelta = Vec2<Unit>{ Unit{ (f32)sdlEvent.wheel.x }, Unit{ (f32)sdlEvent.wheel.y } }
                }
            };
        }
        else if ( sdlEvent.type == SDL_MOUSEMOTION )
        {
            inputEvent = InputEvent{
                .Device = EDeviceID::Mouse,
                .Payload = PointerEvent{
                    .Position = Vec2<Unit>{ Unit{ (f32)sdlEvent.motion.x }, Unit{ (f32)sdlEvent.motion.y } },
                    .Delta = Vec2<Unit>{ Unit{ (f32)sdlEvent.motion.xrel }, Unit{ (f32)sdlEvent.motion.yrel } },
                    .Type = EPointerType::Mouse
                }
            };
        }

        OnInputEvent( inputEvent );
    }
}

bool SDL2Application::Shutdown()
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
