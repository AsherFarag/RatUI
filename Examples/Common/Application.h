#pragma once
#include <RatUI/RatUI.h>
#include <SDL2/SDL.h>

/**
 * @brief The core application class for running RatUI demos and examples.
 *
 * Provides SDL2-backed windowing, event processing and a render loop.
 * Derive from this class and override the four virtual hooks to implement
 * your example.
 */
class Application
{
public:
    /**
     * @brief Configuration parameters for the application window.
     */
    struct Config
    {
        const char* Title  = "RatUI Application";
        int         Width  = 1280;
        int         Height = 720;
    };

    /** @brief Constructs an application with the default window configuration. */
    Application() : Application( Config{} ) {}

    /** @brief Constructs an application with a custom window configuration. */
    explicit Application( Config a_Config ) : m_Config( std::move( a_Config ) ) {}
    virtual ~Application() = default;

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

                SDL_SetRenderDrawColor( m_SDLRenderer, 0, 0, 0, 255 );
                SDL_RenderClear( m_SDLRenderer );
                OnRender();
                SDL_RenderPresent( m_SDLRenderer );
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
    virtual bool OnInitialize() = 0;
    virtual void OnUpdate()     = 0;
    virtual void OnRender()     = 0;
    virtual bool OnShutdown()   = 0;

    /** @brief Returns the underlying SDL_Window handle. */
    SDL_Window*   GetWindow()      const { return m_Window; }

    /** @brief Returns the underlying SDL_Renderer handle. */
    SDL_Renderer* GetSDLRenderer() const { return m_SDLRenderer; }

private:
    bool Initialize()
    {
        if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
            return false;

        m_Window = SDL_CreateWindow(
            m_Config.Title,
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

        return OnInitialize();
    }

    void ProcessEvents()
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            if ( event.type == SDL_QUIT )
                RequestExit();
        }
    }

    bool Shutdown()
    {
        bool result = OnShutdown();

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

private:
    Config        m_Config;
    bool          m_Running{ true };
    SDL_Window*   m_Window{ nullptr };
    SDL_Renderer* m_SDLRenderer{ nullptr };
};