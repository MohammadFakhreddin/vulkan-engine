#include "Application.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/UISystem.hpp"
#include "engine/Scene.hpp"

void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace UI = MFA::UISystem;
    namespace SceneSubSystem = MFA::SceneSubSystem;
    static constexpr MFA::U16 SCREEN_WIDTH = 800;
    static constexpr MFA::U16 SCREEN_HEIGHT = 600;
    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
    UI::Init();
    SceneSubSystem::SetActiveScene("SpecularHighlight");
    SceneSubSystem::Init();
    {// Main loop
        bool quit = false;
        //Event handlerpbt
        SDL_Event e;
        //While application is running
        MFA::U32 const target_fps = 1000 / 60;
        MFA::U32 delta_time = 0;
        while (!quit)
        {
            MFA::U32 const start_time = SDL_GetTicks();
            // DrawFrame
            SceneSubSystem::OnNewFrame(delta_time);
            //Handle events on queue
            if (SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
                // TODO Capture resize event here
            }
            delta_time = SDL_GetTicks() - start_time;
            if(target_fps > delta_time){
                SDL_Delay( target_fps - delta_time );
            }
            delta_time = SDL_GetTicks() - start_time;
        }
    }
    RF::DeviceWaitIdle();
    SceneSubSystem::Shutdown();
    UI::Shutdown();
    RF::Shutdown();
}

