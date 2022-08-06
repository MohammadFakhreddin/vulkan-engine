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
            EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent,
            CameraComponent
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

        void Update(float deltaTimeInSec) override;

        void OnUI() override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Clone(Entity * entity) const override;

    private:

        float mMoveSpeed;
        float mRotationSpeed;

    };

};
