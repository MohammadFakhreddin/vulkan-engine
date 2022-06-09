#pragma once

#include "BedrockPlatforms.hpp"

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#endif

#include <cstdint>

namespace MFA::InputManager {

#if defined(__DESKTOP__)
    using MousePosition = int32_t;
#elif defined(__ANDROID__) || defined(__IOS__)
    using MousePosition = float;
#else
    #error Os is not handled
#endif

    void Init();

    void OnNewFrame(float deltaTimeInSec);

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

    [[nodiscard]]
    bool IsMouseLocationValid();

#ifdef __ANDROID__
    void SetAndroidApp(android_app * androidApp);
#endif

#ifdef __IOS__
    void UpdateTouchState(bool isMouseDown, bool isTouchValueValid, MousePosition mouseX, MousePosition mouseY);
#endif

    void WarpMouseAtEdges(bool warp);

}

namespace MFA {
    namespace IM = InputManager;
}
