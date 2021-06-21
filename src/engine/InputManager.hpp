#pragma once

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#endif

namespace MFA::InputManager {

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

#ifdef __ANDROID__
    void SetInputQueue(AInputQueue * inputQueue_);
#endif

}
