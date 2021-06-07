#include "Application.hpp"

#include "engine/InputManager.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/pbr_scene/PBRScene.hpp"
#include "scenes/textured_sphere/TexturedSphereScene.hpp"
#include "scenes/texture_viewer_scene/TextureViewerScene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/Scene.hpp"

#include "libs/sdl/SDL.hpp"

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
    namespace IM = MFA::InputManager;
    namespace MSDL = MFA::MSDL;
    
    static constexpr uint16_t SCREEN_WIDTH = 800;//1920;
    static constexpr uint16_t SCREEN_HEIGHT = 600;//1080;

    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
    UI::Init();
    IM::Init();
    
    mSceneSubSystem.RegisterNew(mTexturedSphereScene.get(), "TextureSphereScene");
    mSceneSubSystem.RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    mSceneSubSystem.RegisterNew(mPbrScene.get(), "PBRScene");
    mSceneSubSystem.RegisterNew(mTextureViewerScene.get(), "TextureViewerScene");
    
    mSceneSubSystem.SetActiveScene("GLTFMeshViewerScene");
    mSceneSubSystem.Init();

    {// Main loop
        bool quit = false;
        //Event handler
        MSDL::SDL_Event e;
        //While application is running
        uint32_t const targetFps = 1000 / 60;
        uint32_t deltaTime = 0;
        while (!quit)
        {
            uint32_t const start_time = MSDL::SDL_GetTicks();
            IM::OnNewFrame();
            // DrawFrame
            mSceneSubSystem.OnNewFrame(static_cast<float>(deltaTime) / 1000.0f);
            //Handle events
            if (MSDL::SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == MSDL::SDL_QUIT)
                {
                    quit = true;
                }
            }
            deltaTime = MSDL::SDL_GetTicks() - start_time;
            if(targetFps > deltaTime){
                MSDL::SDL_Delay( targetFps - deltaTime );
            }
            deltaTime = MSDL::SDL_GetTicks() - start_time;
        }
    }
    RF::DeviceWaitIdle();
    mSceneSubSystem.Shutdown();
    IM::Shutdown();
    UI::Shutdown();
    RF::Shutdown();
}

