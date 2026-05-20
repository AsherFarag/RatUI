#pragma once
#include <RatUI/RatUI.h>
#include <GL/glew.h>
#include <RatUI/Backends/OpenGL/OpenGLRenderer.h>
#include <SDL2/SDL.h>
#include <memory>

using namespace RatUI;

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
    SDL2Application( Config a_Config = {} );
    virtual ~SDL2Application() = default;

    /** @brief Returns the underlying SDL_Window handle. */
    SDL_Window* GetWindow() const { return m_Window; }

    /** @brief Requests the application to exit the main loop and shut down. */
    void RequestExit() { m_Running = false; }

    /** @brief Checks if the application is still running. */
    bool IsRunning() const { return m_Running; }

    bool Run();

protected:
    virtual bool OnInitialize() { return true; };
    virtual void OnUpdate() {};
    virtual void OnRender( IRenderer& a_Renderer ) {};
    virtual bool OnShutdown() { return true; };
	virtual void OnInputEvent( const RatUI::InputEvent& ) {}

    /** @brief Returns the OpenGL renderer. */
    OpenGL::OpenGLRenderer* GetRenderer() const { return m_Renderer.get(); }

private:
    static EButtonID SDLKeyboardToRatUIButtonID( SDL_Keycode a_Keycode );
    bool Initialize();
    void ProcessEvents();
    bool Shutdown();

protected:
    Config                                     m_Config;
    bool                                       m_Running{ true };
    SDL_Window*                                m_Window{ nullptr };
    SDL_GLContext                              m_GLContext{ nullptr };
    std::unique_ptr<OpenGL::OpenGLRenderer>    m_Renderer;
};
