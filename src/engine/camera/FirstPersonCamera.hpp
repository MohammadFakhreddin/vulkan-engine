#pragma once

#include "ICamera.h"
#include "engine/ui_system/UIRecordObject.hpp"

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
        float moveSpeed = 10.0f,
#if defined(__DESKTOP__) || defined(__IOS__)
        float rotationSpeed = 10.0f
#elif defined(__ANDROID__)
        float rotationSpeed = 15.0f
#else
#error Os is not handled
#endif
    );

    ~FirstPersonCamera() override = default;

    FirstPersonCamera (FirstPersonCamera const &) noexcept = delete;
    FirstPersonCamera (FirstPersonCamera &&) noexcept = delete;
    FirstPersonCamera & operator = (FirstPersonCamera const &) noexcept = delete;
    FirstPersonCamera & operator = (FirstPersonCamera &&) noexcept = delete;

    void init();

    void onNewFrame(float deltaTimeInSec) override;

    void onResize() override;

    void getProjection(float outProjectionMatrix[16]) override;

    [[nodiscard]]
    Matrix4X4Float const & getProjection() const override;

    void getTransform(float outTransformMatrix[16]) override;

    [[nodiscard]]
    Matrix4X4Float const & getTransform() const override;

    void forcePosition(float position[3]) override;

    void forceRotation(float eulerAngles[3]) override;

    void forcePositionAndRotation(Vector3Float const & position, Vector3Float const & eulerAngles);

    void EnableUI(char const * windowName, bool * isVisible) override;

    void DisableUI() override;

    void GetPosition(float position[3]) const;

private:

    void onUI();

    void updateTransform();
    
    float const mFieldOfView;
    float const mFarPlane;
    float const mNearPlane;

    float const mMoveSpeed;
    float const mRotationSpeed;

    Matrix4X4Float mProjectionMatrix {};
    Matrix4X4Float mTransformMatrix {};

    Vector4Float mViewPosition {};
    Vector3Float mEulerAngles {};

    std::string mRecordWindowName {};
    UIRecordObject mRecordUIObject;
    bool * mIsUIVisible = nullptr;

};

};
