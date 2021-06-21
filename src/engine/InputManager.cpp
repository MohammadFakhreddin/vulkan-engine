#include "InputManager.hpp"

#include "BedrockPlatforms.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/UISystem.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

namespace MFA::InputManager {

namespace RF = RenderFrontend;
namespace UI = UISystem;

struct State {
    bool isMouseLocationValid = false;
#ifdef __DESKTOP__
    int32_t mouseCurrentX = 0;
    int32_t mouseCurrentY = 0;
#elif defined(__ANDROID__)
    float mouseCurrentX = 0;
    float mouseCurrentY = 0;
#else
#error Os is not handled
#endif

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

#ifdef __ANDROID__

AInputQueue * inputQueue = nullptr;

void SetInputQueue(AInputQueue * inputQueue_) {
    MFA_ASSERT(inputQueue_ != nullptr);
    inputQueue = inputQueue_;
}

static void handleMotionEvent(AInputEvent * event) {
    float mousePositionX = AMotionEvent_getX(event, 0);
    float mousePositionY = AMotionEvent_getY(event, 0);
    state->isLeftMouseDown = true;
//        if (UISystem::HasFocus() == false) {
//            state->isLeftMouseDown = mouseButtonMask & SDL_BUTTON(SDL_BUTTON_LEFT);
//        }
    if (state->isMouseLocationValid == true) {
        state->mouseDeltaX = mousePositionX - state->mouseCurrentX;
        state->mouseDeltaY = mousePositionY - state->mouseCurrentY;
    } else {
        state->isMouseLocationValid = true;
    }
    state->mouseCurrentX = mousePositionX;
    state->mouseCurrentY = mousePositionY;
}

static void handleKeyEvent(AInputEvent * event) {
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

#endif

void Init() {
    state = new State();   
}

void OnNewFrame() {
    state->reset();
#ifdef __DESKTOP__
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
    if (keys[MSDL::SDL_SCANCODE_W]) {
        state->forwardMove += 1.0f;
    }
    if (keys[MSDL::SDL_SCANCODE_S]) {
        state->forwardMove -= 1.0f;
    }
    if (keys[MSDL::SDL_SCANCODE_D]) {
        state->rightMove += 1.0f;
    }
    if (keys[MSDL::SDL_SCANCODE_A]) {
        state->rightMove -= 1.0f;
    }
#elif defined(__ANDROID__)
    if (inputQueue == nullptr) {
        return;
    }
    state->isLeftMouseDown = false;
    AInputEvent * inputEvent = nullptr;
    while (AInputQueue_hasEvents(inputQueue)) {
        AInputQueue_getEvent(inputQueue, &inputEvent);
        auto const eventType = AInputEvent_getType(inputEvent);
        switch (eventType) {
            case AINPUT_EVENT_TYPE_MOTION: {
                handleMotionEvent(inputEvent);
                break;
            }
            case AINPUT_EVENT_TYPE_KEY: {
                handleKeyEvent(inputEvent);
                break;
            }
            default:
                break;
        }
        AInputQueue_finishEvent(inputQueue, inputEvent, 0);
    }
#else
    #error Os is not supported
#endif
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
