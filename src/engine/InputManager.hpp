#pragma once

#include "BedrockPlatforms.hpp"

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#endif

#include <cstdint>

namespace MFA::InputManager {

#ifdef __DESKTOP__
    using MousePosition = int32_t;
#elif defined(__ANDROID__)
    using MousePosition = float;
#else
    #error Os is not handled
#endif

    void Init();

    void OnNewFrame();

    void Shutdown();

    [[nodiscard]]
    float GetMouseDeltaX();

    [[nodiscard]]
    float GetMouseDeltaY();

    [[nodiscard]]
    float GetForwardMove();

    [[nodiscard]]
    float GetRightMove();

    [[nodiscard]]
    bool IsLeftMouseDown();

    [[nodiscard]]
    MousePosition GetMouseX();

    [[nodiscard]]
    MousePosition GetMouseY();

#ifdef __ANDROID__
    void SetAndroidApp(android_app * androidApp);
#endif

}
