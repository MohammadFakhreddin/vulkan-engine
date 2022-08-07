#pragma once

#include "ColliderComponent.hpp"
#include "engine/entity_system/Component.hpp"

namespace MFA
{

    class CapsuleColliderComponent final : public ColliderComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            CapsuleColliderComponent,
            EventTypes::EmptyEvent,
            ColliderComponent
        )

        explicit CapsuleColliderComponent(
            float halfHeight,
            float radius,
            glm::vec3 const & center = {},
            Physics::SharedHandle<physx::PxMaterial> material = {}
        );

        ~CapsuleColliderComponent() override;

        void OnUI() override;

        void OnTransformChange() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    protected:

        [[nodiscard]]
        std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() override;

        float mHalfHeight = 0.0f;
        float mRadius = 0.0f;

    };

    using CapsuleCollider = CapsuleColliderComponent;

}
