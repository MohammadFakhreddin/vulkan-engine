#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "CameraComponent.hpp"

namespace MFA
{

    class TransformComponent;

    class ObserverCameraComponent final : public CameraComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            ObserverCameraComponent,
            FamilyType::CameraComponent,
            EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent
        )
        
        explicit ObserverCameraComponent(
            float fieldOfView,
            float nearDistance,
            float farDistance,
            float moveSpeed = 10.0f,
#if defined(__DESKTOP__) || defined(__IOS__)
            float rotationSpeed = 10.0f
#elif defined(__ANDROID__)
            float rotationSpeed = 15.0f
#else
#error Os is not handled
#endif
        );

        ~ObserverCameraComponent() override = default;
        
        void Init() override;

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        void OnUI() override;

    private:

        float const mMoveSpeed;
        float const mRotationSpeed;

    };

};
