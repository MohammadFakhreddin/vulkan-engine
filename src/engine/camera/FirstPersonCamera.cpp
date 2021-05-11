#include "FirstPersonCamera.h"

#include "engine/RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;

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

    updateTransform();
    onResize();
}

void FirstPersonCamera::applyInput(float const deltaTime, Input const & input) {

    auto const rotationDistance = mRotationSpeed * deltaTime;
    mEulerAngles.setX(mEulerAngles.getX() - input.mouseDeltaX * rotationDistance);    // Reverse for view mat
    mEulerAngles.setY(mEulerAngles.getY() - input.mouseDeltaY * rotationDistance);    // Reverse for view mat
    // TODO Refactor and clean this part
    Matrix4X4Float rotationMatrix {};
    Matrix4X4Float::AssignRotation(
        rotationMatrix,
        mEulerAngles.getX(),
        mEulerAngles.getY(),
        mEulerAngles.getZ()
    );

    Vector4Float forwardDirection {};
    forwardDirection.assign(ForwardVector);
    forwardDirection.multiply(rotationMatrix);
    Matrix4X1Float::Normalize(forwardDirection);

    Vector4Float rightDirection {};
    rightDirection.assign(rightDirection);
    rightDirection.multiply(rotationMatrix);
    Matrix4X4Float::Normalize(rightDirection);

    auto const moveDistance = mMoveSpeed * deltaTime;

    forwardDirection.multiply(moveDistance * input.forwardBackwardMove);
    mPosition.sum(forwardDirection);

    rightDirection.multiply(moveDistance * input.rightLeftMove);
    mPosition.sum(rightDirection);

}

void FirstPersonCamera::onResize() {
    int32_t width;
    int32_t height;
    RF::GetWindowSize(width, height);
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

void FirstPersonCamera::getTransform(float outTransformMatrix[16]) {
    mTransformMatrix.copy(outTransformMatrix);
}

void FirstPersonCamera::forcePosition(Vector3Float const & position) {
    mPosition.assign(position);
    updateTransform();
}

void FirstPersonCamera::forceRotation(Vector3Float const & eulerAngles) {
    mEulerAngles.assign(eulerAngles);
    updateTransform();
}

void FirstPersonCamera::updateTransform() {
    Matrix4X4Float::Identity(mTransformMatrix);
    Matrix4X4Float::AssignRotation(
        mTransformMatrix,
        mEulerAngles.getX(),
        mEulerAngles.getY(),
        mEulerAngles.getZ()
    );
    Matrix4X4Float::AssignTranslation(
        mTransformMatrix,
        mPosition.getX(),
        mPosition.getY(),
        mPosition.getZ()
    );
}

}
