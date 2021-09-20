#pragma once

#include <fwd.hpp>

#include "CameraBase.hpp"

namespace MFA {

class DrawableVariant;

class ThirdPersonCamera final : public CameraBase {
public:
    // ThirdPerson camera must act like a child to variant
    explicit ThirdPersonCamera(
        float fieldOfView,
        float farPlane,
        float nearPlane,
        DrawableVariant * variant, 
        float distance[3], 
        float eulerAngles[3]
    );

    ~ThirdPersonCamera() override = default;

    ThirdPersonCamera (ThirdPersonCamera const &) noexcept = delete;
    ThirdPersonCamera (ThirdPersonCamera &&) noexcept = delete;
    ThirdPersonCamera & operator = (ThirdPersonCamera const &) noexcept = delete;
    ThirdPersonCamera & operator = (ThirdPersonCamera &&) noexcept = delete;

    void Init();

    void OnUpdate(float deltaTime) override;

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

private:

    void updateTransform();

    float const mFieldOfView;
    float const mFarPlane;
    float const mNearPlane;

    DrawableVariant * mVariant = nullptr;
    float mRelativePosition[3] {};
    float mRelativeEulerAngles[3] {};
    glm::mat4 mTransform {1};
    glm::mat4 mProjection {1};

    bool mIsTransformDirty = true;

};

}
