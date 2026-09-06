#include <stdexcept>

#include "Core/App.h"
#include "Core/Exceptions.h"

void __cdecl main()
{
    // Setup a handler for SEH exceptions to avoid hard crashes.
    SetUnhandledExceptionFilter(Exceptions::HandleSehException);

    try
    {
        // Run the app.
        App app;
        app.Run();
    }
    catch (const std::exception &exception)
    {
        // Intercept any uncaught C++ exception to avoid hard crashes.
        Exceptions::HandleCppException(exception);
    }
}
