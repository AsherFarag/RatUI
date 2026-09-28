#include "Application.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <RatUI/Backends/FreeType/FontLoader.h>
#include <RatUI/Backends/OpenGL/OpenGLRenderer.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <iostream>

namespace
{
    constexpr const char* c_WindowTitle  = "RatUI Examples";
    constexpr int         c_WindowWidth  = 1280;
    constexpr int         c_WindowHeight = 720;

    // Smallest space (in Units) the examples are laid out in. Smaller screens, such as phones,
    // scale the whole UI down to fit instead of cutting it off.
    constexpr f32 c_MinLayoutWidth  = 480.f;
    constexpr f32 c_MinLayoutHeight = 700.f;

    // -------------------------------------------------------------------------
    // SDL -> RatUI conversions
    // -------------------------------------------------------------------------

    EButtonID ToButton( SDL_Keycode a_Key )
    {
        if ( a_Key >= SDLK_a && a_Key <= SDLK_z )
            return static_cast<EButtonID>( (int)EButtonID::KeyA + ( a_Key - SDLK_a ) );
        if ( a_Key >= SDLK_0 && a_Key <= SDLK_9 )
            return static_cast<EButtonID>( (int)EButtonID::Key0 + ( a_Key - SDLK_0 ) );
        if ( a_Key >= SDLK_F1 && a_Key <= SDLK_F12 )
            return static_cast<EButtonID>( (int)EButtonID::KeyF1 + ( a_Key - SDLK_F1 ) );

        switch ( a_Key )
        {
            case SDLK_RETURN:    return EButtonID::KeyEnter;
            case SDLK_ESCAPE:    return EButtonID::KeyEscape;
            case SDLK_SPACE:     return EButtonID::KeySpace;
            case SDLK_TAB:       return EButtonID::KeyTab;
            case SDLK_BACKSPACE: return EButtonID::KeyBackspace;
            case SDLK_UP:        return EButtonID::KeyUp;
            case SDLK_DOWN:      return EButtonID::KeyDown;
            case SDLK_LEFT:      return EButtonID::KeyLeft;
            case SDLK_RIGHT:     return EButtonID::KeyRight;
            default:             return EButtonID::Unknown;
        }
    }

    EButtonID ToMouseButton( Uint8 a_Button )
    {
        switch ( a_Button )
        {
            case SDL_BUTTON_LEFT:   return EButtonID::MouseLeft;
            case SDL_BUTTON_RIGHT:  return EButtonID::MouseRight;
            case SDL_BUTTON_MIDDLE: return EButtonID::MouseMiddle;
            case SDL_BUTTON_X1:     return EButtonID::Mouse3;
            case SDL_BUTTON_X2:     return EButtonID::Mouse4;
            default:                return EButtonID::Unknown;
        }
    }

    EModifier ToModifiers( SDL_Keymod a_Mods )
    {
        EModifier result = EModifier::None;
        if ( a_Mods & KMOD_LSHIFT ) result |= EModifier::LShift;
        if ( a_Mods & KMOD_RSHIFT ) result |= EModifier::RShift;
        if ( a_Mods & KMOD_LCTRL )  result |= EModifier::LCtrl;
        if ( a_Mods & KMOD_RCTRL )  result |= EModifier::RCtrl;
        if ( a_Mods & KMOD_LALT )   result |= EModifier::LAlt;
        if ( a_Mods & KMOD_RALT )   result |= EModifier::RAlt;
        if ( a_Mods & KMOD_LGUI )   result |= EModifier::LSuper;
        if ( a_Mods & KMOD_RGUI )   result |= EModifier::RSuper;
        return result;
    }

    /** @brief Arrow keys, Enter and Escape drive keyboard navigation between widgets. */
    void Navigate( Scene& a_Scene, EButtonID a_Button, bool a_Pressed )
    {
        switch ( a_Button )
        {
            case EButtonID::KeyUp:     if ( a_Pressed ) a_Scene.Navigate( ENavAction::MoveUp );    break;
            case EButtonID::KeyDown:   if ( a_Pressed ) a_Scene.Navigate( ENavAction::MoveDown );  break;
            case EButtonID::KeyLeft:   if ( a_Pressed ) a_Scene.Navigate( ENavAction::MoveLeft );  break;
            case EButtonID::KeyRight:  if ( a_Pressed ) a_Scene.Navigate( ENavAction::MoveRight ); break;
            case EButtonID::KeyEscape: if ( a_Pressed ) a_Scene.Navigate( ENavAction::Cancel );    break;
            case EButtonID::KeyEnter:  a_Scene.Navigate( a_Pressed ? ENavAction::ActivatePressed : ENavAction::ActivateReleased ); break;
            default: break;
        }
    }

    // -------------------------------------------------------------------------
    // Scaling
    //
    // Layout and input work in Units; drawing works in drawable pixels (Unit x GetPixelsPerUnit).
    // -------------------------------------------------------------------------

    /** @brief Drawable pixels per SDL window coordinate (devicePixelRatio in the browser, else 1). */
    f32 GetPixelScale( SDL_Window* a_Window )
    {
        int windowWidth, windowHeight, drawableWidth, drawableHeight;
        SDL_GetWindowSize( a_Window, &windowWidth, &windowHeight );
        SDL_GL_GetDrawableSize( a_Window, &drawableWidth, &drawableHeight );
        return windowWidth > 0 ? static_cast<f32>( drawableWidth ) / windowWidth : 1.f;
    }

    /** @brief Drawable pixels per Unit. */
    f32 GetDPIScale( SDL_Window* a_Window )
    {
#ifdef __EMSCRIPTEN__
        return GetPixelScale( a_Window ); // The canvas is already sized at devicePixelRatio.
#else
        f32 dpi = 96.f;
        if ( SDL_GetDisplayDPI( SDL_GetWindowDisplayIndex( a_Window ), nullptr, &dpi, nullptr ) != 0 )
            dpi = 96.f;
        return dpi / 96.f; // 96 DPI is 1:1.
#endif
    }

    /** @brief Drawable pixels per Unit: the DPI scale, lowered if the layout would be smaller than the minimum size. */
    f32 GetPixelsPerUnit( SDL_Window* a_Window )
    {
        int width, height;
        SDL_GL_GetDrawableSize( a_Window, &width, &height );
        return std::min( { GetDPIScale( a_Window ), width / c_MinLayoutWidth, height / c_MinLayoutHeight } );
    }
}

Application::Application() = default;

Application::~Application() = default;

bool Application::Run( ExampleFactory a_MakeExample )
{
    if ( !Initialize() )
    {
        std::cerr << "RatUI: failed to start: " << SDL_GetError() << "\n";
#ifdef __EMSCRIPTEN__
        EM_ASM( if ( Module.onStartupFailed ) Module.onStartupFailed(); ); // Shows a message on the page (Shell.html).
#endif
        Shutdown();
        return false;
    }

    m_Example   = a_MakeExample( *m_TextMetrics );
    m_LastTicks = SDL_GetTicks64();

#ifdef __EMSCRIPTEN__
    // The browser owns the loop and calls Frame() once per display refresh, so this never returns.
    emscripten_set_main_loop_arg( []( void* a_App ) { static_cast<Application*>( a_App )->Frame(); }, this, 0, true );
#else
    while ( m_Running )
        Frame();

    Shutdown();
#endif
    return true;
}

bool Application::Initialize()
{
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
        return false;

#ifdef __EMSCRIPTEN__
    // OpenGL ES 3.0, which is WebGL2 in the browser.
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
#else
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#endif
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#ifdef __EMSCRIPTEN__
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI; // Gives the canvas a devicePixelRatio-sized backbuffer.
#endif

    m_Window = SDL_CreateWindow( c_WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, c_WindowWidth, c_WindowHeight, windowFlags );
    if ( !m_Window )
        return false;

    m_GLContext = SDL_GL_CreateContext( m_Window );
    if ( !m_GLContext )
        return false;

    SDL_GL_SetSwapInterval( 1 ); // VSync

#ifndef __EMSCRIPTEN__
    if ( glewInit() != GLEW_OK )
    {
        SDL_SetError( "glewInit failed" );
        return false;
    }
#endif

    m_Renderer = MakeUnique<OpenGL::OpenGLRenderer>();
    if ( !m_Renderer->IsValid() )
    {
        SDL_SetError( "OpenGL renderer shaders failed to compile" );
        return false;
    }

    LoadFonts();

    m_TextMetrics = MakeUnique<TextMetrics>( m_Fonts );
    m_Atlas       = MakeUnique<GlyphAtlas>( *m_Renderer, m_Fonts );
    m_DrawList    = MakeUnique<DrawList>( *m_Atlas );
    return true;
}

void Application::LoadFonts()
{
    FreeType::FontLoader loader; // Faces keep FreeType alive, so the loader can be temporary.

    Fonts::Roboto = m_Fonts.RegisterFamily( "Roboto"_id );
    m_Fonts.AddFaceToFamily( Fonts::Roboto, loader.LoadFromFile( "Resources/Fonts/Roboto-Regular.ttf" ),    { EFontWeight::Regular, EFontStyle::Normal } );
    m_Fonts.AddFaceToFamily( Fonts::Roboto, loader.LoadFromFile( "Resources/Fonts/Roboto-Medium.ttf" ),     { EFontWeight::Medium,  EFontStyle::Normal } );
    m_Fonts.AddFaceToFamily( Fonts::Roboto, loader.LoadFromFile( "Resources/Fonts/Roboto-Bold.ttf" ),       { EFontWeight::Bold,    EFontStyle::Normal } );
    m_Fonts.AddFaceToFamily( Fonts::Roboto, loader.LoadFromFile( "Resources/Fonts/Roboto-Italic.ttf" ),     { EFontWeight::Regular, EFontStyle::Italic } );
    m_Fonts.AddFaceToFamily( Fonts::Roboto, loader.LoadFromFile( "Resources/Fonts/Roboto-BoldItalic.ttf" ), { EFontWeight::Bold,    EFontStyle::Italic } );

    // Hinted bitmaps are sharper than MTSDF for small text. No italic face, so italic is synthesized.
    Fonts::RobotoRaster = m_Fonts.RegisterFamily( "Roboto Raster"_id );
    m_Fonts.AddFaceToFamily( Fonts::RobotoRaster, loader.LoadFromFile( "Resources/Fonts/Roboto-Regular.ttf", { .Mode = EGlyphRenderMode::Raster } ), { EFontWeight::Regular } );
    m_Fonts.AddFaceToFamily( Fonts::RobotoRaster, loader.LoadFromFile( "Resources/Fonts/Roboto-Bold.ttf",    { .Mode = EGlyphRenderMode::Raster } ), { EFontWeight::Bold } );

    // Pixel-art TTF, native grid auto-detected.
    Fonts::Minecraft = m_Fonts.RegisterFamily( "Minecraft"_id );
    m_Fonts.AddFaceToFamily( Fonts::Minecraft, loader.LoadFromFile( "Resources/Fonts/Minecraft.ttf", { .Mode = EGlyphRenderMode::Pixel } ) );

    // Characters Roboto lacks fall back to the pixel font.
    m_Fonts.SetFallbacks( Fonts::Roboto, { Fonts::Minecraft } );
    m_Fonts.SetDefaultFamily( Fonts::Roboto );
}

void Application::Shutdown()
{
    // Everything holding GPU resources goes before the GL context.
    m_Example.reset();
    m_DrawList.reset();
    m_Atlas.reset();
    m_TextMetrics.reset();
    m_Renderer.reset();

    if ( m_GLContext )
        SDL_GL_DeleteContext( m_GLContext );
    if ( m_Window )
        SDL_DestroyWindow( m_Window );

    m_GLContext = nullptr;
    m_Window    = nullptr;
    SDL_Quit();
}

void Application::Frame()
{
    ProcessEvents();

    const u64 ticks        = SDL_GetTicks64();
    const f32 deltaSeconds = ( ticks - m_LastTicks ) / 1000.f;
    m_LastTicks = ticks;

    int width, height;
    SDL_GL_GetDrawableSize( m_Window, &width, &height );
    const f32 pixelsPerUnit = GetPixelsPerUnit( m_Window );

    // Update and lay out in Units...
    Scene& scene = m_Example->GetScene();
    m_Example->Update( deltaSeconds );
    scene.UpdateLayout( { Unit{ width / pixelsPerUnit }, Unit{ height / pixelsPerUnit } } );
    scene.Tick( deltaSeconds );

    // ...then draw in pixels.
    glViewport( 0, 0, width, height );
    m_Renderer->SetViewport( width, height );

    const Vec4f clear = ToColorF32( Colors::Surface900 );
    glClearColor( clear[0], clear[1], clear[2], clear[3] );
    glClear( GL_COLOR_BUFFER_BIT );

    m_DrawList->SetDPIScale( pixelsPerUnit );
    m_DrawList->Clear();
    m_DrawList->SetDebugEnabled( m_DebugDraw );
    m_Example->Render( *m_DrawList, deltaSeconds );
    m_DrawList->Flush( *m_Renderer );

    SDL_GL_SwapWindow( m_Window );
}

void Application::ProcessEvents()
{
    Scene& scene = m_Example->GetScene();

    // SDL reports window coordinates; RatUI input is in Units.
    const f32  windowToUnits = GetPixelScale( m_Window ) / GetPixelsPerUnit( m_Window );
    const auto toUnits       = [windowToUnits]( Sint32 a_X, Sint32 a_Y ) { return Vec2<Unit>{ Unit{ a_X * windowToUnits }, Unit{ a_Y * windowToUnits } }; };

    SDL_Event event;
    while ( SDL_PollEvent( &event ) )
    {
        const EModifier modifiers = ToModifiers( SDL_GetModState() );

        switch ( event.type )
        {
            case SDL_QUIT:
                m_Running = false;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                // F1 toggles the debug overlay; it is not passed on to the scene.
                if ( event.key.keysym.sym == SDLK_F1 )
                {
                    if ( event.type == SDL_KEYDOWN && !event.key.repeat )
                        m_DebugDraw = !m_DebugDraw;
                    break;
                }

                const EButtonID button  = ToButton( event.key.keysym.sym );
                const bool      pressed = event.type == SDL_KEYDOWN;
                Navigate( scene, button, pressed );
                scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard,
                                            .Payload = ButtonEvent{ .Button = button, .Modifiers = modifiers, .Pressed = pressed, .Released = !pressed } } );
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                const bool pressed = event.type == SDL_MOUSEBUTTONDOWN;
                scene.DispatchInputEvent( { .Device = EDeviceID::Mouse,
                                            .Payload = ButtonEvent{ .Button          = ToMouseButton( event.button.button ),
                                                                    .Modifiers       = modifiers,
                                                                    .Pressed         = pressed,
                                                                    .Released        = !pressed,
                                                                    .Pointer         = PointerID{ 0 },
                                                                    .PointerPosition = toUnits( event.button.x, event.button.y ) } } );
                break;
            }

            case SDL_MOUSEMOTION:
                scene.DispatchInputEvent( { .Device = EDeviceID::Mouse,
                                            .Payload = PointerEvent{ .Position  = toUnits( event.motion.x, event.motion.y ),
                                                                     .Delta     = toUnits( event.motion.xrel, event.motion.yrel ),
                                                                     .Type      = EPointerType::Mouse,
                                                                     .Modifiers = modifiers } } );
                break;

            case SDL_MOUSEWHEEL:
                scene.DispatchInputEvent( { .Device = EDeviceID::Mouse,
                                            .Payload = PointerEvent{ .Type        = EPointerType::Mouse,
                                                                     .Modifiers   = modifiers,
                                                                     .ScrollDelta = Vec2<Unit>{ Unit{ (f32)event.wheel.x }, Unit{ (f32)event.wheel.y } } } } );
                break;

            default:
                break;
        }
    }
}
