#include "Application.hpp"

#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/pbr_scene/PBRScene.hpp"
#include "scenes/textured_sphere/TexturedSphereScene.hpp"
#include "scenes/texture_viewer_scene/TextureViewerScene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/Scene.hpp"

#include <SDL2/SDL.h>

Application::Application()
    : mGltfMeshViewerScene(std::make_unique<GLTFMeshViewerScene>())
    , mPbrScene(std::make_unique<PBRScene>())
    , mTexturedSphereScene(std::make_unique<TexturedSphereScene>())
    , mTextureViewerScene(std::make_unique<TextureViewerScene>())
{}

Application::~Application() = default;


void Application::run() {
    namespace RF = MFA::RenderFrontend;
    namespace UI = MFA::UISystem;
    
    static constexpr uint16_t SCREEN_WIDTH = 800;//1920;
    static constexpr uint16_t SCREEN_HEIGHT = 600;//1080;

    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
    UI::Init();
    
    mSceneSubSystem.RegisterNew(mTexturedSphereScene.get(), "TextureSphereScene");
    mSceneSubSystem.RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    mSceneSubSystem.RegisterNew(mPbrScene.get(), "PBRScene");
    mSceneSubSystem.RegisterNew(mTextureViewerScene.get(), "TextureViewerScene");
    
    mSceneSubSystem.SetActiveScene("GLTFMeshViewerScene");
    mSceneSubSystem.Init();

    {// Main loop
        bool quit = false;
        //Event handler
        SDL_Event e;
        //While application is running
        uint32_t const target_fps = 1000 / 60;
        uint32_t delta_time = 0;
        while (!quit)
        {
            uint32_t const start_time = SDL_GetTicks();
            // DrawFrame
            mSceneSubSystem.OnNewFrame(delta_time);
            //Handle events
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
    mSceneSubSystem.Shutdown();
    UI::Shutdown();
    RF::Shutdown();
}

