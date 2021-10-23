#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "CameraComponent.hpp"

namespace MFA
{

    class TransformComponent;

    class ObserverCameraComponent final : public CameraComponent
    {
    public:

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

        ObserverCameraComponent(ObserverCameraComponent const &) noexcept = delete;
        ObserverCameraComponent(ObserverCameraComponent &&) noexcept = delete;
        ObserverCameraComponent & operator = (ObserverCameraComponent const &) noexcept = delete;
        ObserverCameraComponent & operator = (ObserverCameraComponent &&) noexcept = delete;

        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::ObserverCameraComponent;
            return 1;
        }

        void Init() override;

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        void OnUI() override;

    private:

        void updateTransform();

        float const mMoveSpeed;
        float const mRotationSpeed;

        glm::mat4 mTransformMatrix{};

        bool mIsTransformDirty = false;
    };

};
