#include <Application.h>
#include <SDL2Renderer.h>

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
    bool OnInitialize() override
    {
        m_Renderer = std::make_unique<SDL2Renderer>( GetSDLRenderer() );
        return true;
    }

    void OnUpdate() override
    {
        // TODO: Update the RatUI widget tree
    }

    void OnRender() override
    {
        // The Application base class has already cleared the back-buffer to
        // black before this call.  Draw custom content here.
        SDL_Renderer* sdl = m_Renderer->GetSDLRenderer();

        // Draw a simple placeholder rectangle so that the sandbox window shows
        // something visible from the first frame.
        SDL_SetRenderDrawColor( sdl, 80, 120, 200, 255 );
        SDL_Rect rect{ 40, 40, 200, 100 };
        SDL_RenderFillRect( sdl, &rect );
    }

    bool OnShutdown() override
    {
        m_Renderer.reset();
        return true;
    }

private:
    std::unique_ptr<SDL2Renderer> m_Renderer;
};

int main( int /*argc*/, char* /*argv*/[] )
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
