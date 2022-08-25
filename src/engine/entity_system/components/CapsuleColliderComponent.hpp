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
            Physics::SharedHandle<physx::PxMaterial> material = {}
        );

        ~CapsuleColliderComponent() override;

        void OnUI() override;

        void OnTransformChange() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    private:

        void RotateToDirection();

        void UI_ShapeUpdate();

        void UI_CapsuleDirection();

        void ComputeCenter();

        void OnCapsuleGeometryChanged();

        void OnCapsuleDirectionChanged();

    protected:

        [[nodiscard]]
        std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() override;

        MFA_ATOMIC_VARIABLE2(HalfHeight, float, 0.0f, OnCapsuleGeometryChanged)

        MFA_ATOMIC_VARIABLE2(Radius, float, 0.0f, OnCapsuleDirectionChanged)
        
        CapsuleDirection mCapsuleDirection = defaultDirection;

    };

    using CapsuleCollider = CapsuleColliderComponent;

}
