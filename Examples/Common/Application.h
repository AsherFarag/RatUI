#pragma once
#include <RatUI/RatUI.h>

/**
 * @brief The core testing application class for running RatUI demos and examples.
 * TODO: Implement SDL or other proper application framework
 */
class Application
{
public:
    Application() = default;
    virtual ~Application() = default;

    /** @brief Requests the application to exit the main loop and shut down. */
    void RequestExit() { m_Running = false; }

    /** @brief Checks if the application is still running. */
    bool IsRunning() const { return m_Running; }

    bool Run()
    {
        if ( !Initialize() )
        {
            // Initialization failed, handle as needed (e.g., log error, throw exception, etc.)
            return false;
        }

        // Main loop
        try
        {       
            while ( IsRunning() )
            {
                OnUpdate();
                OnRender();
            }
        }
        catch ( const std::exception& e )
        {
            // Handle exceptions from the main loop as needed (e.g., log error, clean up resources, etc.)
            return false;
        }
        catch ( ... )
        {
            // Handle any other types of exceptions as needed
            return false;
        }

        return Shutdown();
    }

protected:
    virtual bool OnInitialize() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual bool OnShutdown() = 0;

private:

    bool Initialize() { return OnInitialize(); }
    bool Shutdown() { return OnShutdown(); }

private:
    bool m_Running{ true };

};