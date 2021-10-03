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

        void Update(float deltaTimeInSec) override;

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

        float mPosition[3] {};
        float mEulerAngles[3] {0.0f, 0.0f, 0.0f};

        glm::mat4 mProjectionMatrix{};
        glm::mat4 mTransformMatrix{};

        bool mIsTransformDirty = false;
    };

};
