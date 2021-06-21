#include "Application.hpp"

#include "engine/InputManager.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/pbr_scene/PBRScene.hpp"
#include "scenes/textured_sphere/TexturedSphereScene.hpp"
#include "scenes/texture_viewer_scene/TextureViewerScene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/Scene.hpp"
#include "engine/BedrockFileSystem.hpp"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

namespace RF = MFA::RenderFrontend;
namespace UI = MFA::UISystem;
namespace IM = MFA::InputManager;
namespace FS = MFA::FileSystem;
#ifdef __DESKTOP__
namespace MSDL = MFA::MSDL;
#endif


Application::Application()
    : mGltfMeshViewerScene(std::make_unique<GLTFMeshViewerScene>())
    , mPbrScene(std::make_unique<PBRScene>())
    , mTexturedSphereScene(std::make_unique<TexturedSphereScene>())
    , mTextureViewerScene(std::make_unique<TextureViewerScene>())
{}

Application::~Application() = default;

void Application::Init() {
    static constexpr uint16_t SCREEN_WIDTH = 1200;//1920;
    static constexpr uint16_t SCREEN_HEIGHT = 800;//1080;

    MFA_ASSERT(mIsInitialized == false);

    {
        RF::InitParams params{};
        params.applicationName = "MfaEngine";
    #ifdef __ANDROID__
        params.app = mAndroidApp;
    #elif defined(__DESKTOP__)
        params.resizable = true;
        params.screenWidth = SCREEN_WIDTH;
        params.screenHeight = SCREEN_HEIGHT;
    #else
        #error Os not supported
    #endif
        RF::Init(params);
    }
    UI::Init();
    IM::Init();
    
    mSceneSubSystem.RegisterNew(mTexturedSphereScene.get(), "TextureSphereScene");
    mSceneSubSystem.RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    mSceneSubSystem.RegisterNew(mPbrScene.get(), "PBRScene");
    mSceneSubSystem.RegisterNew(mTextureViewerScene.get(), "TextureViewerScene");
    
    mSceneSubSystem.SetActiveScene("GLTFMeshViewerScene");
    mSceneSubSystem.Init();

    mIsInitialized = true;
}

void Application::Shutdown() {
    MFA_ASSERT(mIsInitialized == true);

    RF::DeviceWaitIdle();
    mSceneSubSystem.Shutdown();
    IM::Shutdown();
    UI::Shutdown();
    RF::Shutdown();

    mIsInitialized = false;
}

void Application::run() {
#ifdef __DESKTOP__
    Init();
    {// Main loop
        bool quit = false;
        //Event handler
        MSDL::SDL_Event e;
        //While application is running
        uint32_t const targetFps = 1000 / 120;
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
    Shutdown();
#elif defined(__ANDROID__)
    // Used to poll the events in the main loop
    int events;
    android_poll_source* source;
    //While application is running
    uint32_t const targetFps = 1000 / 120;
    uint32_t deltaTime = 1;

    while (mAndroidApp->destroyRequested == 0) {
        if (ALooper_pollAll(mIsInitialized ? 1 : 0, nullptr,
                         &events, (void**)&source) >= 0) {
            if (source != nullptr) source->process(mAndroidApp, source);
        }
        if (mIsInitialized == false) {
            continue;
        }

        IM::OnNewFrame();
        // DrawFrame
        mSceneSubSystem.OnNewFrame(static_cast<float>(deltaTime) / 1000.0f);
        // // render if vulkan is ready
        // if (IsVulkanReady()) {
        //   VulkanDrawFrame();
        // }
    };
#else
#error "Platform is not supported"
#endif

}

#ifdef __ANDROID__
void Application::setAndroidApp(android_app * androidApp) {
    mAndroidApp = androidApp;
    MFA::FileSystem::SetAndroidApp(androidApp);
}
#endif

