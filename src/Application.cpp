#include "Application.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/InputManager.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include <thread>
#endif

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

using namespace MFA;


Application::Application()
    : mThirdPersonDemoScene(nullptr)
    , mGltfMeshViewerScene(nullptr)
{}

Application::~Application() = default;

void Application::Init() {
    {// Initializing render frontend
        RF::InitParams params{};
        params.applicationName = "MfaEngine";
    #ifdef __ANDROID__
        params.app = mAndroidApp;
    #elif defined(__IOS__)
        params.view = mView;
    #elif defined(__DESKTOP__)
        auto const screenInfo = MFA::Platforms::ComputeScreenSize();
        auto const screenWidth = screenInfo.screenWidth;
        auto const screenHeight = screenInfo.screenHeight;

        MFA_ASSERT(mIsInitialized == false);


        params.resizable = true;
        params.screenWidth = screenWidth;
        params.screenHeight = screenHeight;
    #else
        #error Os not supported
    #endif
        RF::Init(params);
    }
    UI::Init();
    IM::Init();
    JS::Init();
    EntitySystem::Init();
    
    SceneManager::Init();

    mThirdPersonDemoScene = std::make_unique<Demo3rdPersonScene>();
    mGltfMeshViewerScene = std::make_unique<GLTFMeshViewerScene>();

    SceneManager::RegisterNew(mThirdPersonDemoScene.get(), "ThirdPersonDemoScene");
    SceneManager::RegisterNew(mGltfMeshViewerScene.get(), "GLTFMeshViewerScene");
    
    SceneManager::SetActiveScene("ThirdPersonDemoScene");

    mIsInitialized = true;
}

void Application::Shutdown() {
    MFA_ASSERT(mIsInitialized == true);

    RF::DeviceWaitIdle();
    SceneManager::Shutdown();
    EntitySystem::Shutdown();
    JS::Shutdown();
    IM::Shutdown();
    UI::Shutdown();
    RF::Shutdown();

    mIsInitialized = false;
}

void Application::run() {
    static constexpr uint32_t TargetFpsDeltaTimeInMs = 1000 / 1000;
#ifdef __DESKTOP__
    Init();
    {// Main loop
        //Event handler
        MSDL::SDL_Event e;
        //While application is running
        uint32_t deltaTimeInMs = 0;
        while (true)
        {
            uint32_t const start_time = MSDL::SDL_GetTicks();
            //Handle events
            if (MSDL::SDL_PollEvent(&e) != 0)
            {
                //User requests quit
                if (e.type == MSDL::SDL_QUIT)
                {
                    break;
                }
            }
            RenderFrame(static_cast<float>(deltaTimeInMs) / 1000.0f);
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

void Application::RenderFrame(float const deltaTimeInSec) {
    IM::OnNewFrame(deltaTimeInSec);
    SceneManager::OnNewFrame(deltaTimeInSec);
    RF::OnNewFrame(deltaTimeInSec);
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
