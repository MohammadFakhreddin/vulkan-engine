#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/physics/PhysicsTypes.hpp"

#ifdef __ANDROID__
struct android_app;
#endif

class Application {
public:
    Application();
    virtual ~Application();

    Application & operator= (Application && rhs) noexcept = delete;
    Application (Application const &) noexcept = delete;
    Application (Application && rhs) noexcept = delete;
    Application & operator = (Application const &) noexcept = delete;

    void Init();
    void Shutdown();

#ifdef __ANDROID__
    void setAndroidApp(android_app * androidApp);
#endif
#ifdef __IOS__
    void SetView(void * view);
#endif
    void run();
    void RenderFrame(float rawDeltaTime);

protected:

    virtual void internalInit() {};
    virtual void internalShutdown() {};
    virtual void internalRenderFrame(float deltaTimeInSec) {};
    virtual MFA::RT::FrontendInitParams GetRenderFrontendInitParams();
    virtual MFA::Physics::InitParams GetPhysicsInitParams();

private:

    bool mIsInitialized = false;

#ifdef __ANDROID__
    android_app * mAndroidApp = nullptr;
#endif
#ifdef __IOS__
  void * mView = nullptr;
#endif
};
