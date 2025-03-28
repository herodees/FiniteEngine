#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES3)
#include "GLFW/glfw3.h"
#endif


#include "include.hpp"

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES3)
#if defined(__APPLE__)
    glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_METAL);
#elif defined(_WIN32)
    glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_D3D11);
#endif
#endif
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Finite");

    rlImGuiSetup(true);

    int targetFPS = 60;
    float fixedFPS = 60.0f;
    float maxFPS = 1200.0f;
    double timeCounter = 0.0;
    double fixedTimeStep = 1.0f / fixedFPS;
    double maxTimeStep = 1.0f / maxFPS;
    double currentTime = GetTime();
    double accumulator = 0.0;

    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------

        double newTime = GetTime();
        double frameTime = newTime - currentTime;

        // Avoid spiral of death if CPU  can't keep up with target FPS
        if (frameTime > 0.25)
        {
            frameTime = 0.25;
        }

        currentTime = newTime;

        // Limit frame rate to avoid 100% CPU usage
        if (frameTime < maxTimeStep)
        {
            float waitTime = (float)(maxTimeStep - frameTime);
            WaitTime(1.0f / 1000.0f);
        }

        // Input
        //----------------------------------------------------------------------------------

        PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

        accumulator += frameTime;
        while (accumulator >= fixedTimeStep)
        {
            // Fixed Update
            //--------------------------------------------------------------------------

            timeCounter += fixedTimeStep; // We count time (seconds)

            timeCounter += fixedTimeStep;
            accumulator -= fixedTimeStep;
        }

        // Drawing
        //----------------------------------------------------------------------------------

        BeginDrawing();

        ClearBackground(GRAY);

        rlImGuiBegin(frameTime);

        ImGui::ShowDemoWindow();

        rlImGuiEnd();

        EndDrawing();

        SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)
    }

    // Cleanup
    //-------------------------------------------------------------------------------------

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
