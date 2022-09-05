#include "InputManager.hpp"

#include "BedrockMath.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system//UI_System.hpp"
#include "engine/BedrockAssert.hpp"

#ifdef __DESKTOP__
#include "libs/sdl/SDL.hpp"
#endif

#ifdef __ANDROID__
#include <queue>
#endif

// TODO: OOP with inheritance was much more cleaner

namespace MFA::InputManager {

#ifdef __ANDROID__
inline static constexpr float TouchEndDelayInSec = 10.0f / 1000.0f;
#endif

struct State {
    bool isMouseLocationValid = false;
    MousePosition mouseCurrentX = 0;
    MousePosition mouseCurrentY = 0;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float forwardMove = 0.0f;
    float rightMove = 0.0f;
    bool isLeftMouseDown = false;
    bool isRightMouseDown = false;
#ifdef __ANDROID__
    float lastTimeSinceTouchInSec = 0;
#endif

    bool warpMouseAtEdges = false; // Desktop only

    int recordUI_Id;

    void reset() {
        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
        forwardMove = 0.0f;
        rightMove = 0.0f;
        isLeftMouseDown = false;
        isRightMouseDown = false;
    }
};

State * state = nullptr;

#ifdef __ANDROID__

static void handleMotionEvent(AInputEvent * event) {
    float mousePositionX = AMotionEvent_getX(event, 0);
    float mousePositionY = AMotionEvent_getY(event, 0);
    if (mousePositionX < 0 || mousePositionY < 0) {
        // Maybe also check width and height
        state->isMouseLocationValid = false;
        return;
    }
    // TODO Create time class
    state->lastTimeSinceTouchInSec = static_cast<float>(clock()) / CLOCKS_PER_SEC;
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
//    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

static void HandleInputEvent(AInputEvent * event) {
    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_MOTION: {
            handleMotionEvent(event);
            break;
        }
        case AINPUT_EVENT_TYPE_KEY: {
            handleKeyEvent(event);
            break;
        }
        default:
            break;
    }
}

// Based on https://groups.google.com/g/android-ndk/c/-pqmAFs9Xck
static int32_t OnInputEvent(android_app * app, AInputEvent * event) {
    int32_t isHandled = 0;
    if (state != nullptr) {
        isHandled = 1;
        MFA_ASSERT(event != nullptr);
        HandleInputEvent(event);
    }
    return isHandled;

}

void SetAndroidApp(android_app * androidApp) {
    MFA_ASSERT(androidApp != nullptr);
    androidApp->onInputEvent = OnInputEvent;
}

#endif

static void onUI() {
#if defined(__ANDROID__) || defined(__IOS__)
    state->forwardMove = 0.0f;
    state->rightMove = 0.0f;
    UI::BeginWindow("Input controller");

    UI::Button("Forward", []()->void {});
    if (UI::IsItemActive()) {
        state->forwardMove += 1.0f;
    }
    UI::Spacing();

    UI::Button("Right", []()->void {});
    if (UI::IsItemActive()) {
        state->rightMove += 1.0f;
    }
    UI::Spacing();

    UI::Button("Left", []()->void {});
    if (UI::IsItemActive()) {
        state->rightMove -= 1.0f;
    }
    UI::Spacing();

    UI::Button("Backward", []()->void {});
    if (UI::IsItemActive()) {
        state->forwardMove -= 1.0f;
    }
    UI::Spacing();


    auto const moveLength = std::sqrt(state->forwardMove * state->forwardMove + state->rightMove * state->rightMove);
    if (moveLength > Math::Epsilon<float>())
    {
        state->rightMove /= moveLength;
        state->forwardMove /= moveLength;
    }


    UI::EndWindow();
#endif
}

void Init() {

    state = new State {
        .recordUI_Id = UI::Register([]()->void{onUI();})
    };
}

void OnNewFrame(float deltaTimeInSec) {
#ifdef __DESKTOP__
    state->reset();
    int32_t mousePositionX = 0;
    int32_t mousePositionY = 0;
    auto const mouseButtonMask = RF::GetMouseState(&mousePositionX, &mousePositionY);
    state->isLeftMouseDown = mouseButtonMask & SDL_BUTTON(SDL_BUTTON_LEFT);
    state->isRightMouseDown = mouseButtonMask & SDL_BUTTON(SDL_BUTTON_RIGHT);
    if (state->isMouseLocationValid == true) {
        state->mouseDeltaX = static_cast<float>(mousePositionX - state->mouseCurrentX);
        state->mouseDeltaY = static_cast<float>(mousePositionY - state->mouseCurrentY);
    } else {
        state->isMouseLocationValid = true;
    }
    state->mouseCurrentX = mousePositionX;
    state->mouseCurrentY = mousePositionY;

    if (state->warpMouseAtEdges) {
        auto const surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const screenWidth = surfaceCapabilities.currentExtent.width;
        auto const screenHeight = surfaceCapabilities.currentExtent.height;

        bool mousePositionNeedsWarping = false;
        if (state->mouseCurrentX < static_cast<int>(static_cast<float>(screenWidth) * 0.010f)) {
            state->mouseCurrentX = static_cast<int>(static_cast<float>(screenWidth) * 0.010f) + 50;
            mousePositionNeedsWarping = true;
        }
        if (state->mouseCurrentX > static_cast<int>(static_cast<float>(screenWidth) * 0.990f)) {
            state->mouseCurrentX = static_cast<int>(static_cast<float>(screenWidth) * 0.990f) - 50;
            mousePositionNeedsWarping = true;
        }
        if (state->mouseCurrentY < static_cast<int>(static_cast<float>(screenHeight) * 0.010f)) {
            state->mouseCurrentY = static_cast<int>(static_cast<float>(screenHeight) * 0.010f) + 50;
            mousePositionNeedsWarping = true;
        }
        if (state->mouseCurrentY > static_cast<int>(static_cast<float>(screenHeight) * 0.990f)) {
            state->mouseCurrentY = static_cast<int>(static_cast<float>(screenHeight) * 0.990f) - 50;
            mousePositionNeedsWarping = true;
        }
        if (mousePositionNeedsWarping) {
            RF::WarpMouseInWindow(state->mouseCurrentX, state->mouseCurrentY);
        }
    }
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

    auto const moveLength = std::sqrt(state->forwardMove * state->forwardMove + state->rightMove * state->rightMove);
    if (moveLength > Math::Epsilon<float>())
    {
        state->rightMove /= moveLength;
        state->forwardMove /= moveLength;
    }

#elif defined(__ANDROID__)
    state->isLeftMouseDown = false;
    float const nowInSec = static_cast<float>(clock()) / CLOCKS_PER_SEC;
    if (nowInSec - state->lastTimeSinceTouchInSec < TouchEndDelayInSec) {
        state->isLeftMouseDown = true;
    } else {
        state->isMouseLocationValid = false;
        state->mouseCurrentX = -1.0f;
        state->mouseCurrentY = -1.0f;
    }
#elif defined(__IOS__)
#else
    #error Os is not supported
#endif
}

void Shutdown() {
    UI::UnRegister(state->recordUI_Id);
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

bool IsRightMouseDown()
{
    return state->isRightMouseDown;
}

MousePosition GetMouseX() {
    return state->mouseCurrentX;
}

MousePosition GetMouseY() {
    return state->mouseCurrentY;
}

bool IsMouseLocationValid() {
    return state->isMouseLocationValid;
}

#ifdef __IOS__
void UpdateTouchState(bool isMouseDown, bool isTouchValueValid, MousePosition mouseX, MousePosition mouseY) {
    if (state->isMouseLocationValid && isTouchValueValid) {
        state->mouseDeltaX = mouseX - state->mouseCurrentX;
        state->mouseDeltaY = mouseY - state->mouseCurrentY;
    } else {
        state->mouseDeltaX = 0.0f;
        state->mouseDeltaY = 0.0f;
    }
    state->mouseCurrentX = mouseX;
    state->mouseCurrentY = mouseY;
    state->isLeftMouseDown = isMouseDown;
    state->isMouseLocationValid = isTouchValueValid && isMouseDown;
    // MFA_LOG_INFO("Mouse: DeltaX: %f, DeltaY: %f", state->mouseDeltaX, state->mouseDeltaY);
}
#endif

void WarpMouseAtEdges(bool const warp) {
    state->warpMouseAtEdges = warp;
}

}
