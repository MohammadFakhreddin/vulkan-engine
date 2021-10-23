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

    void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

    void Shutdown() override;

    void OnUI() override;

private:

    TransformComponent * mTransformComponent = nullptr;
    int mTransformChangeListenerId = 0;

    float mDistance = 3.0f;
    
    float mRotationSpeed;

};

}
