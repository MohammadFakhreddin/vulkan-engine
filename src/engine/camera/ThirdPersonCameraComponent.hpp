#pragma once

#include "CameraComponent.hpp"
#include "engine/BedrockPlatforms.hpp"

namespace MFA
{

    class PBR_Variant;
    class TransformComponent;

    class ThirdPersonCameraComponent final : public CameraComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            ThirdPersonCameraComponent,
            EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent,
            CameraComponent
        )

        static constexpr float DefaultRotationSpeed = 20.0f;
        static constexpr float DefaultDistance = 3.0f;
        static constexpr bool DefaultWrapMouseAtEdges = false;
        static constexpr glm::vec3 DefaultExtraOffset{ 0.0f, -1.0f, 0.0f };

        // ThirdPerson camera must act like a child to variant
        explicit ThirdPersonCameraComponent(
            std::shared_ptr<TransformComponent> const & followTarget,
            float fieldOfView,
            float nearDistance,
            float farDistance,
            float rotationSpeed = DefaultRotationSpeed,
            float distance = DefaultDistance,
            bool wrapMouseAtEdges = DefaultWrapMouseAtEdges,
            glm::vec3 extraOffset = DefaultExtraOffset
        );

        ~ThirdPersonCameraComponent() override = default;

        void Init() override;

        void Update(float deltaTimeInSec) override;

        void Shutdown() override;

        void OnUI() override;

        void SetFollowTarget(std::shared_ptr<TransformComponent> const & followTarget);

    private:

        void OnFollowTargetMove() const;

        void OnWrapMouseAtEdgesChange() const;

        std::weak_ptr<TransformComponent> mFollowTarget{};

        MFA_ATOMIC_VARIABLE2(
            Distance,
            float,
            DefaultDistance,
            OnFollowTargetMove
        )
        
        MFA_ATOMIC_VARIABLE1(
            RotationSpeed,
            float,
            DefaultRotationSpeed
        )

        MFA_ATOMIC_VARIABLE2(
            WrapMouseAtEdges,
            bool,
            DefaultWrapMouseAtEdges,
            OnWrapMouseAtEdgesChange
        )

        MFA_VECTOR_VARIABLE2(
            ExtraOffset,
            glm::vec3,
            DefaultExtraOffset,
            OnFollowTargetMove
        )

        SignalId mFollowTargetListenerId = SignalIdInvalid;

        bool mHasFocus = false;

    };

}
