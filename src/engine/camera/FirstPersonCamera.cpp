#include "FirstPersonCamera.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/ui_system/UISystem.hpp"

#include "glm/gtx/quaternion.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace IM = InputManager;
namespace UI = UISystem;

FirstPersonCamera::FirstPersonCamera(
    float const fieldOfView,
    float const farPlane,
    float const nearPlane,
    Vector3Float const & position, 
    Vector3Float const & eulerAngles,
    float const moveSpeed,
    float const rotationSpeed
)
    : mFieldOfView(fieldOfView)
    , mFarPlane(farPlane)
    , mNearPlane(nearPlane)
    , mMoveSpeed(moveSpeed)
    , mRotationSpeed(rotationSpeed)
    , mRecordUIObject([this]()->void{onUI();})
{
    mViewPosition.assign(position);
    mEulerAngles.assign(eulerAngles);
}

void FirstPersonCamera::init() {
    updateTransform();
    onResize();
}

void FirstPersonCamera::onNewFrame(float const deltaTimeInSec) {
    if (InputManager::IsLeftMouseDown() == true && UISystem::HasFocus() == false) {
        auto const mouseDeltaX = IM::GetMouseDeltaX();
        auto const mouseDeltaY = IM::GetMouseDeltaY();
        auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
        mEulerAngles.setY(mEulerAngles.getY() - mouseDeltaX * rotationDistance);    // Reverse for view mat
        mEulerAngles.setX(Math::Clamp(
            mEulerAngles.getX() - mouseDeltaY * rotationDistance,
            -90.0f,
            90.0f
        ));    // Reverse for view mat
    }

    auto const rotationMatrix = glm::toMat4(glm::quat(glm::vec3(
        glm::radians(180.0f - mEulerAngles.getX()),
        glm::radians(180.0f - mEulerAngles.getY()),
        glm::radians(180.0f - mEulerAngles.getZ())
    )));
    
    auto forwardDirection = glm::vec4(
        ForwardVector.getX(), 
        ForwardVector.getY(), 
        ForwardVector.getZ(), 
        ForwardVector.getW()
    );
    forwardDirection = rotationMatrix * forwardDirection;
    forwardDirection = glm::normalize(forwardDirection);
    
    auto rightDirection = glm::vec4(
        RightVector.getX(), 
        RightVector.getY(), 
        RightVector.getZ(), 
        RightVector.getW()
    );
    rightDirection = rotationMatrix * rightDirection;
    rightDirection = glm::normalize(rightDirection);
    
    auto const forwardMove = IM::GetForwardMove();
    auto const rightMove = -1.0f * IM::GetRightMove();
    auto const moveDistance = mMoveSpeed * deltaTimeInSec;
    
    mViewPosition.setX(mViewPosition.getX() + forwardDirection.x * moveDistance * forwardMove);
    mViewPosition.setY(mViewPosition.getY() + forwardDirection.y * moveDistance * forwardMove);
    mViewPosition.setZ(mViewPosition.getZ() + forwardDirection.z * moveDistance * forwardMove);
    mViewPosition.setW(0.0f);

    mViewPosition.setX(mViewPosition.getX() + rightDirection.x * moveDistance * rightMove);
    mViewPosition.setY(mViewPosition.getY() + rightDirection.y * moveDistance * rightMove);
    mViewPosition.setZ(mViewPosition.getZ() + rightDirection.z * moveDistance * rightMove);
    mViewPosition.setW(0.0f);

    updateTransform();
}

void FirstPersonCamera::onResize() {
    int32_t width;
    int32_t height;
    RF::GetDrawableSize(width, height);
    MFA_ASSERT(width > 0);
    MFA_ASSERT(height > 0);

    const float ratio = static_cast<float>(width) / static_cast<float>(height);

    Matrix4X4Float::PreparePerspectiveProjectionMatrix(
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
//    Matrix4X4Float::ConvertMatrixToGlm(mProjectionMatrix, projectionMatrix);
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
//    Matrix4X4Float::ConvertGmToCells(projectionMatrix, mProjectionMatrix.cells);
//#endif
}

void FirstPersonCamera::GetProjection(float outProjectionMatrix[16]) {
    mProjectionMatrix.copy(outProjectionMatrix);  
}

Matrix4X4Float const & FirstPersonCamera::GetProjection() const {
    return mProjectionMatrix;
}

void FirstPersonCamera::GetTransform(float outTransformMatrix[16]) {
    mTransformMatrix.copy(outTransformMatrix);
}

Matrix4X4Float const & FirstPersonCamera::getTransform() const {
    return mTransformMatrix;
}

void FirstPersonCamera::forcePosition(float position[3]) {
    mViewPosition.assign(position, 3);
    updateTransform();
}

void FirstPersonCamera::forceRotation(float eulerAngles[3]) {
    mEulerAngles.assign(eulerAngles);
    updateTransform();
}

void FirstPersonCamera::forcePositionAndRotation(
    Vector3Float const & position, 
    Vector3Float const & eulerAngles
) {
    mViewPosition.assign(position);
    mEulerAngles.assign(eulerAngles);
    updateTransform();
}

void FirstPersonCamera::EnableUI(char const * windowName, bool * isVisible) {
    MFA_ASSERT(windowName != nullptr);
    mRecordWindowName = "Camera" + std::string(windowName);
    mIsUIVisible = isVisible;
    mRecordUIObject.Enable();
}

void FirstPersonCamera::DisableUI() {
    mRecordUIObject.Disable();
}

void FirstPersonCamera::GetPosition(float position[3]) const {
    position[0] = -1 * mViewPosition.cells[0];
    position[1] = -1 * mViewPosition.cells[1];
    position[2] = -1 * mViewPosition.cells[2];
}

void FirstPersonCamera::onUI() {
    if (*mIsUIVisible == true) {
        UI::BeginWindow(mRecordWindowName.c_str());
        UI::InputFloat3("Position", mViewPosition.cells);
        UI::InputFloat3("EulerAngles", mEulerAngles.cells);
        UI::EndWindow();
    }
}

void FirstPersonCamera::updateTransform() {
    Matrix4X4Float::Identity(mTransformMatrix);

    Matrix4X4Float rotationMatrix {};
    Matrix4X4Float::AssignRotation(
        rotationMatrix,
        mEulerAngles.getX(),
        mEulerAngles.getY(),
        mEulerAngles.getZ()
    );

    Matrix4X4Float translationMatrix {};
    Matrix4X4Float::AssignTranslation(
        translationMatrix,
        mViewPosition.getX(),
        mViewPosition.getY(),
        mViewPosition.getZ()
    );

    mTransformMatrix.multiply(rotationMatrix);
    mTransformMatrix.multiply(translationMatrix);
}

}
