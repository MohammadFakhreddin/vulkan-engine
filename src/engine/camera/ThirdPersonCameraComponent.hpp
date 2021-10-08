#pragma once

#include <fwd.hpp>

#include "CameraComponent.hpp"
#include "engine/BedrockPlatforms.hpp"

namespace MFA {

class DrawableVariant;
class TransformComponent;

class ThirdPersonCameraComponent final : public CameraComponent {
public:
    // ThirdPerson camera must act like a child to variant
    explicit ThirdPersonCameraComponent(
        float fieldOfView,
        float nearDistance,
        float farDistance,
#if defined(__DESKTOP__) || defined(__IOS__)
        float rotationSpeed = 20.0f
#elif defined(__ANDROID__)
        float rotationSpeed = 30.0f
#else
#error Os is not handled
#endif
    );

    ~ThirdPersonCameraComponent() override = default;

    static uint8_t GetClassType(ClassType outComponentTypes[3])
    {
        // TODO It should be reverse CameraComponent must return all its child 
        outComponentTypes[0] = ClassType::ThirdPersonCamera;
        return 1;
    }

    void SetDistanceAndRotation(
        float distance, 
        float eulerAngles[3]
    );

    void Init() override;

    void Update(float deltaTimeInSec) override;

    void Shutdown() override;

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

    TransformComponent * mTransformComponent = nullptr;
    int mTransformChangeListenerId = 0;

    float mDistance = 3.0f;
    glm::mat4 mTransform {1};
    
    bool mIsTransformDirty = true;

    float mRotationSpeed;

};

}
