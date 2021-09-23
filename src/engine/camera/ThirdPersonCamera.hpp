#pragma once

#include <fwd.hpp>

#include "CameraBase.hpp"
#include "engine/BedrockPlatforms.hpp"

namespace MFA {

class DrawableVariant;

class ThirdPersonCamera final : public CameraBase {
public:
    // ThirdPerson camera must act like a child to variant
    explicit ThirdPersonCamera(
        float fieldOfView,
        float farPlane,
        float nearPlane,
#if defined(__DESKTOP__) || defined(__IOS__)
        float rotationSpeed = 20.0f
#elif defined(__ANDROID__)
        float rotationSpeed = 30.0f
#else
#error Os is not handled
#endif
    );

    ~ThirdPersonCamera() override = default;

    ThirdPersonCamera (ThirdPersonCamera const &) noexcept = delete;
    ThirdPersonCamera (ThirdPersonCamera &&) noexcept = delete;
    ThirdPersonCamera & operator = (ThirdPersonCamera const &) noexcept = delete;
    ThirdPersonCamera & operator = (ThirdPersonCamera &&) noexcept = delete;

    void Init(
        DrawableVariant * variant, 
        float distance, 
        float eulerAngles[3]
    );

    void OnUpdate(float deltaTimeInSec) override;

    void OnResize() override;

    void GetProjection(float outProjectionMatrix[16]) override;

    [[nodiscard]]
    glm::mat4 const & GetProjection() const override;

    void GetTransform(float outTransformMatrix[16]) override;

    [[nodiscard]]
    glm::mat4 const & GetTransform() const override;

    void ForcePosition(float position[3]) override;

    void ForceRotation(float eulerAngles[3]) override;

    void GetPosition(float outPosition[3]) const override;

    void GetRotation(float outEulerAngles[3]) const;

    void OnUI() override;

private:

    void updateTransform();

    float const mFieldOfView;
    float const mFarPlane;
    float const mNearPlane;

    DrawableVariant * mVariant = nullptr;
    float mPosition[3] {};
    float mEulerAngles[3] {};
    float mDistance = 0.0f;
    glm::mat4 mTransform {1};
    glm::mat4 mProjection {};

    bool mIsTransformDirty = true;

    float mRotationSpeed;

};

}
