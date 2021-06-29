#include "FirstPersonCamera.hpp"

#include "engine/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/UISystem.hpp"

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
{
    mPosition.assign(position);
    mEulerAngles.assign(eulerAngles);
}

void FirstPersonCamera::init() {
    updateTransform();
    onResize();
}

void FirstPersonCamera::onNewFrame(float const deltaTimeInSec) {
    if (InputManager::IsLeftMouseDown() == true) {
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
    
    mPosition.setX(mPosition.getX() + forwardDirection.x * moveDistance * forwardMove);
    mPosition.setY(mPosition.getY() + forwardDirection.y * moveDistance * forwardMove);
    mPosition.setZ(mPosition.getZ() + forwardDirection.z * moveDistance * forwardMove);
    mPosition.setW(0.0f);

    mPosition.setX(mPosition.getX() + rightDirection.x * moveDistance * rightMove);
    mPosition.setY(mPosition.getY() + rightDirection.y * moveDistance * rightMove);
    mPosition.setZ(mPosition.getZ() + rightDirection.z * moveDistance * rightMove);
    mPosition.setW(0.0f);

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
}

void FirstPersonCamera::getProjection(float outProjectionMatrix[16]) {
    mProjectionMatrix.copy(outProjectionMatrix);  
}

Matrix4X4Float const & FirstPersonCamera::getProjection() const {
    return mProjectionMatrix;
}

void FirstPersonCamera::getTransform(float outTransformMatrix[16]) {
    mTransformMatrix.copy(outTransformMatrix);
}

Matrix4X4Float const & FirstPersonCamera::getTransform() const {
    return mTransformMatrix;
}

void FirstPersonCamera::forcePosition(float position[3]) {
    mPosition.assign(position, 3);
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
    mPosition.assign(position);
    mEulerAngles.assign(eulerAngles);
    updateTransform();
}

void FirstPersonCamera::onUI() {
    UI::BeginWindow("Camera");
    UI::InputFloat3("Position", mPosition.cells);
    UI::InputFloat3("EulerAngles", mEulerAngles.cells);
    UI::EndWindow();
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
        mPosition.getX(),
        mPosition.getY(),
        mPosition.getZ()
    );

    mTransformMatrix.multiply(rotationMatrix);
    mTransformMatrix.multiply(translationMatrix);
}

}
