#include "ObserverCamera.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/ui_system/UISystem.hpp"

#include "glm/gtx/quaternion.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace IM = InputManager;
namespace UI = UISystem;

//-------------------------------------------------------------------------------------------------

ObserverCamera::ObserverCamera(
    float const fieldOfView,
    float const farPlane,
    float const nearPlane,
    float const moveSpeed,
    float const rotationSpeed
)
    : mFieldOfView(fieldOfView)
    , mFarPlane(farPlane)
    , mNearPlane(nearPlane)
    , mMoveSpeed(moveSpeed)
    , mRotationSpeed(rotationSpeed)
{}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::Init() {
    updateTransform();
    OnResize();
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::OnUpdate(float const deltaTimeInSec) {

    if (mIsTransformDirty) {
        updateTransform();
    }

    bool mRotationIsChanged = false;
    if (InputManager::IsLeftMouseDown() == true && UISystem::HasFocus() == false) {

        auto const mouseDeltaX = IM::GetMouseDeltaX();
        auto const mouseDeltaY = IM::GetMouseDeltaY();

        if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f) {

            auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
            mEulerAngles[1] = mEulerAngles[1] + mouseDeltaX * rotationDistance;    // Reverse for view mat
            mEulerAngles[0] = Math::Clamp(
                mEulerAngles[0] - mouseDeltaY * rotationDistance,
                -90.0f,
                90.0f
            );    // Reverse for view mat

            mRotationIsChanged = true;

        }
    }

    auto const forwardMove = IM::GetForwardMove();
    auto const rightMove = -1.0f * IM::GetRightMove();

    bool mPositionIsChanged = false;
    if (forwardMove != 0.0f || rightMove != 0.0f) {
        mPositionIsChanged = true;
    }

    if (mPositionIsChanged == false && mRotationIsChanged == false) {
        return;
    }

    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::GlmRotate(rotationMatrix, mEulerAngles);
    
    auto forwardDirection = glm::vec4(
        ForwardVector[0], 
        ForwardVector[1], 
        ForwardVector[2], 
        ForwardVector[3]
    );
    forwardDirection = forwardDirection * rotationMatrix;
    forwardDirection = glm::normalize(forwardDirection);
    
    auto rightDirection = glm::vec4(
        RightVector[0], 
        RightVector[1], 
        RightVector[2], 
        RightVector[3]
    );
    rightDirection = rightDirection * rotationMatrix;
    rightDirection = glm::normalize(rightDirection);
    
    auto const moveDistance = mMoveSpeed * deltaTimeInSec;

    if (forwardMove != 0.0f) {
        mPosition[0] = mPosition[0] + forwardDirection[0] * moveDistance * forwardMove;
        mPosition[1] = mPosition[1] + forwardDirection[1] * moveDistance * forwardMove;
        mPosition[2] = mPosition[2] + forwardDirection[2] * moveDistance * forwardMove;
    }
    if (rightMove != 0.0f) {
        mPosition[0] = mPosition[0] + rightDirection[0] * moveDistance * rightMove;
        mPosition[1] = mPosition[1] + rightDirection[1] * moveDistance * rightMove;
        mPosition[2] = mPosition[2] + rightDirection[2] * moveDistance * rightMove;
    }
    updateTransform();
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::OnResize() {
    int32_t width;
    int32_t height;
    RF::GetDrawableSize(width, height);
    MFA_ASSERT(width > 0);
    MFA_ASSERT(height > 0);

    const float ratio = static_cast<float>(width) / static_cast<float>(height);

    Matrix::PreparePerspectiveProjectionMatrix(
        mProjectionMatrix,
        ratio,
        mFieldOfView,
        mNearPlane,
        mFarPlane
    );

// TODO We should handle orientation change on android/IOS (UI shader might need a change)
// https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html
//#ifdef __ANDROID__
//    // For mobile
//    glm::mat4 projectionMatrix;
//    glm::mat4::ConvertMatrixToGlm(mProjectionMatrix, projectionMatrix);
//
//    static constexpr glm::vec3 rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
//    // TODO Should we cache it or it is not important ?
//    auto const capabilities = RF::GetSurfaceCapabilities();
//
//    if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
//        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(90.0f), rotationAxis);
//    } else if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
//        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(270.0f), rotationAxis);
//    } else if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
//        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(180.0f), rotationAxis);
//    }
//
//    glm::mat4::ConvertGmToCells(projectionMatrix, mProjectionMatrix.cells);
//#endif
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::GetProjection(float outProjection[16]) {
    Matrix::CopyGlmToCells(mProjectionMatrix, outProjection);
}

//-------------------------------------------------------------------------------------------------

glm::mat4 const & ObserverCamera::GetProjection() const {
    return mProjectionMatrix;
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::GetTransform(float outTransformMatrix[16]) {
    Matrix::CopyGlmToCells(mTransformMatrix, outTransformMatrix);
}

//-------------------------------------------------------------------------------------------------

glm::mat4 const & ObserverCamera::GetTransform() const {
    return mTransformMatrix;
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::ForcePosition(float position[3]) {
    if (IsEqual<3>(position, mPosition) == false) {
        Copy<3>(mPosition, position);
        updateTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::ForceRotation(float eulerAngles[3]) {
    if (IsEqual<3>(eulerAngles, mEulerAngles) == false) {
        Copy<3>(mEulerAngles, eulerAngles);
        updateTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::GetPosition(float outPosition[3]) const {
    outPosition[0] = -1 * mPosition[0];
    outPosition[1] = -1 * mPosition[1];
    outPosition[2] = -1 * mPosition[2];
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::OnUI() {
    UI::BeginWindow(mName.c_str());
    UI::InputFloat3("Position", mPosition);
    UI::InputFloat3("EulerAngles", mEulerAngles);
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

void ObserverCamera::updateTransform() {
    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::GlmRotate(rotationMatrix, mEulerAngles);

    auto translateMatrix = glm::identity<glm::mat4>();
    Matrix::GlmTranslate(translateMatrix, mPosition);

    mTransformMatrix = rotationMatrix * translateMatrix;

    mIsTransformDirty = false;
}

//-------------------------------------------------------------------------------------------------

}
