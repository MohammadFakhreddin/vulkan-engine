#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "CameraBase.hpp"

namespace MFA {

class ObserverCamera final : public CameraBase
{
public:
    
    explicit ObserverCamera(
        float fieldOfView,
        float farPlane,
        float nearPlane,
        float moveSpeed = 10.0f,
#if defined(__DESKTOP__) || defined(__IOS__)
        float rotationSpeed = 10.0f
#elif defined(__ANDROID__)
        float rotationSpeed = 15.0f
#else
#error Os is not handled
#endif
    );

    ~ObserverCamera() override = default;

    ObserverCamera (ObserverCamera const &) noexcept = delete;
    ObserverCamera (ObserverCamera &&) noexcept = delete;
    ObserverCamera & operator = (ObserverCamera const &) noexcept = delete;
    ObserverCamera & operator = (ObserverCamera &&) noexcept = delete;

    void Init();

    void OnUpdate(float deltaTimeInSec) override;

    void OnResize() override;

    void GetProjection(float outProjection[16]) override;

    [[nodiscard]]
    glm::mat4 const & GetProjection() const override;

    void GetTransform(float outTransformMatrix[16]) override;

    [[nodiscard]]
    glm::mat4 const & GetTransform() const override;

    void ForcePosition(float position[3]) override;

    void ForceRotation(float eulerAngles[3]) override;

    void GetPosition(float outPosition[3]) const override;

    void OnUI() override;

private:

    void updateTransform();
    
    float const mFieldOfView;
    float const mFarPlane;
    float const mNearPlane;

    float const mMoveSpeed;
    float const mRotationSpeed;

    glm::mat4 mProjectionMatrix {};
    glm::mat4 mTransformMatrix {};

    float mPosition[3] {};
    float mEulerAngles[3] {};

    bool mIsTransformDirty = false;

};

};
