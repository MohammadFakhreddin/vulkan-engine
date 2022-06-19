#include "Application.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/InputManager.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

#ifdef __ANDROID__
#include "engine/BedrockFileSystem.hpp"

#include <android_native_app_glue.h>
#include <thread>
#endif

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Application::Application() = default;

//-------------------------------------------------------------------------------------------------

Application::~Application() = default;

//-------------------------------------------------------------------------------------------------

void Application::Init() {

    Path::Init();
    RC::Init();
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
        params.screenWidth = static_cast<RF::ScreenWidth>(static_cast<float>(screenWidth) * 0.9f);
        params.screenHeight = static_cast<RF::ScreenHeight>(static_cast<float>(screenHeight) * 0.9f);
    #else
        #error Os not supported
    #endif
        RF::Init(params);
    }
    JS::Init();
    UI::Init();
    IM::Init();
    EntitySystem::Init();

    SceneManager::Init();

    internalInit();

    mIsInitialized = true;
    
}

//-------------------------------------------------------------------------------------------------

void Application::Shutdown() {
    MFA_ASSERT(mIsInitialized == true);

    RF::DeviceWaitIdle();

    internalShutdown();

    JS::Shutdown();
    SceneManager::Shutdown();
    EntitySystem::Shutdown();
    IM::Shutdown();
    UI::Shutdown();
    RC::Shutdown();
    RF::Shutdown();
    Path::Shutdown();

    mIsInitialized = false;
}

//-------------------------------------------------------------------------------------------------

void Application::run() {
    //static constexpr uint32_t TargetFpsDeltaTimeInMs = 1000 / 500;
#ifdef __DESKTOP__
    Init();
    // Main loop
    //Event handler
    MSDL::SDL_Event e;
    //While application is running
    uint32_t deltaTimeInMs = 0;

    uint32_t startTime = MSDL::SDL_GetTicks();
    while (true)
    {
        //Handle events
        if (MSDL::SDL_PollEvent(&e) != 0)
        {
            //User requests quit
            if (e.type == MSDL::SDL_QUIT)
            {
                break;
            }
        }
        RenderFrame(std::max(static_cast<float>(deltaTimeInMs), 1.0f) / 1000.0f);
        deltaTimeInMs = MSDL::SDL_GetTicks() - startTime;
        startTime = MSDL::SDL_GetTicks();
        /*if(TargetFpsDeltaTimeInMs > deltaTimeInMs){
            MSDL::SDL_Delay( TargetFpsDeltaTimeInMs - deltaTimeInMs);
        }
        deltaTimeInMs = MSDL::SDL_GetTicks() - start_time;*/
    }
    Shutdown();
#elif defined(__ANDROID__)

    static float TargetFpsDeltaTimeInSec = 1.0f / 120.0f;

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

//-------------------------------------------------------------------------------------------------

void Application::RenderFrame(float const deltaTimeInSec) {
    internalRenderFrame(deltaTimeInSec);
    IM::OnNewFrame(deltaTimeInSec);
    SceneManager::Render(deltaTimeInSec);
    SceneManager::Update(deltaTimeInSec);
    RF::OnNewFrame(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

#ifdef __ANDROID__
void Application::setAndroidApp(android_app * androidApp) {
    mAndroidApp = androidApp;
    MFA::FileSystem::SetAndroidApp(androidApp);
    MFA::InputManager::SetAndroidApp(androidApp);
    MFA::UI_System::SetAndroidApp(androidApp);
}
#endif

//-------------------------------------------------------------------------------------------------

#ifdef __IOS__
void Application::SetView(void * view) {
    mView = view;
}
#endif

//-------------------------------------------------------------------------------------------------
