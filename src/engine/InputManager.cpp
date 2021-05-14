#include "InputManager.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/UISystem.hpp"

namespace MFA::InputManager {

namespace RF = RenderFrontend;
namespace UI = UISystem;

struct State {
    bool isMouseLocationValid = false;
    int32_t mouseCurrentX = 0;
    int32_t mouseCurrentY = 0;

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float forwardMove = 0.0f;
    float rightMove = 0.0f;
    bool isLeftMouseDown = false;
    void reset() {
        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
        forwardMove = 0.0f;
        rightMove = 0.0f;
        isLeftMouseDown = false;
    }
};

State * state = nullptr;

void Init() {
    state = new State();   
}

void OnNewFrame() {
    state->reset();

    int32_t mousePositionX = 0;
    int32_t mousePositionY = 0;
    auto const mouseButtonMask = RF::GetMouseState(&mousePositionX, &mousePositionY);

    if (UISystem::HasFocus() == false) {
        state->isLeftMouseDown = mouseButtonMask & SDL_BUTTON(SDL_BUTTON_LEFT);
    }

    if (state->isMouseLocationValid == true) {
        state->mouseDeltaX = static_cast<float>(mousePositionX - state->mouseCurrentX);
        state->mouseDeltaY = static_cast<float>(mousePositionY - state->mouseCurrentY);
    } else {
        state->isMouseLocationValid = true;
    }
    state->mouseCurrentX = mousePositionX;
    state->mouseCurrentY = mousePositionY;

    auto const * keys = RF::GetKeyboardState();
    if (keys[SDL_SCANCODE_W]) {
        state->forwardMove += 1.0f;
    }
    if (keys[SDL_SCANCODE_S]) {
        state->forwardMove -= 1.0f;
    }
    if (keys[SDL_SCANCODE_D]) {
        state->rightMove += 1.0f;
    }
    if (keys[SDL_SCANCODE_A]) {
        state->rightMove -= 1.0f;   
    }
    
}

void Shutdown() {
    delete state;
    state = nullptr;
}

[[nodiscard]]
float GetMouseDeltaX() {
    return state->mouseDeltaX;
}

[[nodiscard]]
float GetMouseDeltaY() {
    return state->mouseDeltaY;
}

[[nodiscard]]
float GetForwardMove() {
    return state->forwardMove;
}

[[nodiscard]]
float GetRightMove() {
    return state->rightMove;
}

[[nodiscard]]
bool IsLeftMouseDown() {
    return state->isLeftMouseDown;
}

}
