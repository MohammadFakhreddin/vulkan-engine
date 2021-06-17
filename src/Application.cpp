#include "Application.hpp"

#include "engine/InputManager.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/pbr_scene/PBRScene.hpp"
#include "scenes/textured_sphere/TexturedSphereScene.hpp"
#include "scenes/texture_viewer_scene/TextureViewerScene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/Scene.hpp"

#include "libs/sdl/SDL.hpp"
#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

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

    static constexpr uint16_t SCREEN_WIDTH = 1200;//1920;
    static constexpr uint16_t SCREEN_HEIGHT = 800;//1080;

    RF::Init({SCREEN_WIDTH, SCREEN_HEIGHT, "Cool app"});
    UI::Init();
    IM::Init();
    
    mSceneSubSystem.RegisterNew(mTexturedSphereScene.get(), "TextureSphereScene");
    mSceneSubSystem.RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    mSceneSubSystem.RegisterNew(mPbrScene.get(), "PBRScene");
    mSceneSubSystem.RegisterNew(mTextureViewerScene.get(), "TextureViewerScene");
    
    mSceneSubSystem.SetActiveScene("GLTFMeshViewerScene");
    mSceneSubSystem.Init();
#ifdef __DESKTOP__
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
#elif defined(__ANDROID__)
    // Used to poll the events in the main loop
    int events;
    android_poll_source* source;
    do {
        // TODO
        // if (ALooper_pollAll(IsVulkanReady() ? 1 : 0, nullptr,
        //                     &events, (void**)&source) >= 0) {
        //   if (source != NULL) source->process(app, source);
        // }

        // // render if vulkan is ready
        // if (IsVulkanReady()) {
        //   VulkanDrawFrame();
        // }
    } while (mAndroidApp->destroyRequested == 0);
#else
#error "Platform is not supported"
#endif
    RF::DeviceWaitIdle();
    mSceneSubSystem.Shutdown();
    IM::Shutdown();
    UI::Shutdown();
    RF::Shutdown();
}

#ifdef __ANDROID__
void Application::setAndroidApp(android_app * androidApp) {
    mAndroidApp = androidApp;
}
#endif

