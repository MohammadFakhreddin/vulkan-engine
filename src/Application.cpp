#include "Application.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/InputManager.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/Scene.hpp"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include <thread>
#endif

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

namespace RF = MFA::RenderFrontend;
namespace UI = MFA::UISystem;
namespace IM = MFA::InputManager;
#ifdef __DESKTOP__
namespace MSDL = MFA::MSDL;
#endif


Application::Application()
    : mThirdPersonDemoScene(std::make_unique<Demo3rdPersonScene>())
    , mGltfMeshViewerScene(std::make_unique<GLTFMeshViewerScene>())
{}

Application::~Application() = default;

void Application::Init() {
    static constexpr uint16_t SCREEN_WIDTH = 1920;//1200;//1920;
    static constexpr uint16_t SCREEN_HEIGHT = 1080;//800;//1080;

    MFA_ASSERT(mIsInitialized == false);

    {
        RF::InitParams params{};
        params.applicationName = "MfaEngine";
    #ifdef __ANDROID__
        params.app = mAndroidApp;
    #elif defined(__IOS__)
        params.view = mView;
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

    mSceneSubSystem.RegisterNew(mThirdPersonDemoScene.get(), "ThirdPersonDemoScene");
    mSceneSubSystem.RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    
    mSceneSubSystem.SetActiveScene("ThirdPersonDemoScene");
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
    static constexpr uint32_t TargetFpsDeltaTimeInMs = 1000 / 120;
//    static constexpr float TargetFpsDeltaTimeInSec = 1.0f / 120.0f;
#ifdef __DESKTOP__
    Init();
    {// Main loop
        bool quit = false;
        //Event handler
        MSDL::SDL_Event e;
        //While application is running
        uint32_t deltaTimeInMs = 0;
        while (!quit)
        {
            uint32_t const start_time = MSDL::SDL_GetTicks();
            RenderFrame(static_cast<float>(deltaTimeInMs) / 1000.0f);
            //Handle events
            if (MSDL::SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == MSDL::SDL_QUIT)
                {
                    quit = true;
                }
            }
            deltaTimeInMs = MSDL::SDL_GetTicks() - start_time;
            if(TargetFpsDeltaTimeInMs > deltaTimeInMs){
                MSDL::SDL_Delay( TargetFpsDeltaTimeInMs - deltaTimeInMs);
            }
            deltaTimeInMs = MSDL::SDL_GetTicks() - start_time;
        }
    }
    Shutdown();
#elif defined(__ANDROID__)
    // Used to poll the events in the main loop
    int events;
    android_poll_source* source;
    //While application is running

    clock_t deltaTimeClock = 0;
    clock_t startTime;
    while (mAndroidApp->destroyRequested == 0) {
        startTime = clock();
        if (ALooper_pollAll(mIsInitialized ? 1 : 0, nullptr,
                         &events, (void**)&source) >= 0) {
            if (source != nullptr) source->process(mAndroidApp, source);
        }
        if (mIsInitialized == false) {
            continue;
        }

        RenderFrame(static_cast<float>(deltaTimeClock) / CLOCKS_PER_SEC);
        deltaTimeClock = clock() - startTime;
        auto const deltaTimeInSec = static_cast<float>(deltaTimeClock) / CLOCKS_PER_SEC;
        if (TargetFpsDeltaTimeInSec > deltaTimeInSec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<u_int32_t>(1000 * (TargetFpsDeltaTimeInSec - deltaTimeInSec))));
            deltaTimeClock = clock() - startTime;
        }
    };
#elif defined(__IOS__)
#else
#error "Platform is not supported"
#endif

}

void Application::RenderFrame(float deltaTimeInSec) {
    IM::OnNewFrame();
    mSceneSubSystem.OnNewFrame(deltaTimeInSec);
}

#ifdef __ANDROID__
void Application::setAndroidApp(android_app * androidApp) {
    mAndroidApp = androidApp;
    MFA::FileSystem::SetAndroidApp(androidApp);
    MFA::InputManager::SetAndroidApp(androidApp);
    MFA::UISystem::SetAndroidApp(androidApp);
}
#endif

#ifdef __IOS__
void Application::SetView(void * view) {
    mView = view;
}
#endif
