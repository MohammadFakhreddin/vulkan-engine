#pragma once

#include "ICamera.h"

namespace MFA {

class FirstPersonCamera final : public ICamera
{
public:

    explicit FirstPersonCamera(
        float fieldOfView,
        float farPlane,
        float nearPlane,
        Vector3Float const & position = Vector3Float {0.0f, 0.0f, 0.0f}, 
        Vector3Float const & eulerAngles = Vector3Float {0.0f, 0.0f, 0.0f},
        float moveSpeed = 0.01f,
        float rotationSpeed = 0.01f
    );

    ~FirstPersonCamera() override = default;

    FirstPersonCamera (FirstPersonCamera const &) noexcept = delete;
    FirstPersonCamera (FirstPersonCamera &&) noexcept = delete;
    FirstPersonCamera & operator = (FirstPersonCamera const &) noexcept = delete;
    FirstPersonCamera & operator = (FirstPersonCamera &&) noexcept = delete;

    void init();

    void onNewFrame(float deltaTime) override;

    void onResize() override;

    void getProjection(float outProjectionMatrix[16]) override;

    void getTransform(float outTransformMatrix[16]) override;

    void forcePosition(Vector3Float const & position) override;

    void forceRotation(Vector3Float const & eulerAngles) override;

    void forcePositionAndRotation(Vector3Float const & position, Vector3Float const & eulerAngles);

private:

    void updateTransform();
    
    float const mFieldOfView;
    float const mFarPlane;
    float const mNearPlane;

    float const mMoveSpeed;
    float const mRotationSpeed;

    Matrix4X4Float mProjectionMatrix {};
    Matrix4X4Float mTransformMatrix {};

    Vector4Float mPosition {};
    Vector3Float mEulerAngles {};

   

};

}