#pragma once

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
    
}
