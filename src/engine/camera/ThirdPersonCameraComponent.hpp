#pragma once

#include <fwd.hpp>

#include "CameraComponent.hpp"
#include "engine/BedrockPlatforms.hpp"

namespace MFA {

class DrawableVariant;
class TransformComponent;

class ThirdPersonCameraComponent final : public CameraComponent {
public:

    MFA_COMPONENT_PROPS(ThirdPersonCameraComponent)
    MFA_COMPONENT_CLASS_TYPE_1(ClassType::ThirdPersonCamera)

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

    void SetDistanceAndRotation(
        float distance, 
        float eulerAngles[3]
    );

    void Init() override;

    void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

    void Shutdown() override;

    void OnUI() override;

private:

    std::weak_ptr<TransformComponent> mTransformComponent {};
    int mTransformChangeListenerId = 0;

    float mDistance = 3.0f;
    
    float mRotationSpeed;

};

}
