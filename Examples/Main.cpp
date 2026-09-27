#include "Application/Application.h"
#include "ThemeShowcase.h"

// To add an example: derive from Example (see ThemeShowcase.h) and run it here instead.
int main( int, char** )
{
    // Static because in the browser main() returns while the application keeps running.
    static Application app;
    return app.Run<ThemeShowcase>() ? 0 : 1;
}
