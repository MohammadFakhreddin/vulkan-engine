#pragma once

#include "ColliderComponent.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Component.hpp"

namespace MFA
{

    class CapsuleColliderComponent final : public ColliderComponent
    {
    public:

        enum class CapsuleDirection
        {
            X,
            Y,
            Z
        };

        static constexpr CapsuleDirection defaultDirection = CapsuleDirection::Y;

        MFA_COMPONENT_PROPS(
            CapsuleColliderComponent,
            EventTypes::EmptyEvent,
            ColliderComponent
        )

        explicit CapsuleColliderComponent(
            float halfHeight,
            float radius,
            CapsuleDirection direction = defaultDirection,
            glm::vec3 const & center = {},
            Physics::SharedHandle<physx::PxMaterial> material = {}
        );

        ~CapsuleColliderComponent() override;

        void OnUI() override;

        void OnTransformChange() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    private:

        [[nodiscard]]
        static glm::quat RotateToDirection(CapsuleDirection direction);

        void UI_ShapeUpdate();

        void UI_CapsuleDirection();

    protected:

        [[nodiscard]]
        std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() override;

        float mHalfHeight = 0.0f;
        float mRadius = 0.0f;

        CapsuleDirection mCapsuleDirection = defaultDirection;

    };

    using CapsuleCollider = CapsuleColliderComponent;

}
