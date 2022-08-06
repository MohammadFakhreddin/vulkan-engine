#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/Physics.hpp"

namespace MFA
{
    class TransformComponent;
    class ColliderComponent;

    class RigidbodyComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            RigidbodyComponent,
            FamilyType::Rigidbody,
            EventTypes::LateInitEvent | EventTypes::UpdateEvent,
            Component
        )

        explicit RigidbodyComponent(
            bool isKinematic,
            bool useGravity,
            bool mass
        );

        void LateInit() override;

        void Update(float deltaTimeInSec) override;

        void OnUI() override;

        void Clone(Entity * entity) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Serialize(nlohmann::json & jsonObject) const override;

    private:

        void UpdateGravity() const;
        void UpdateKinematic() const;
        void UpdateMass() const;

        std::weak_ptr<TransformComponent> mTransform;

        std::weak_ptr<ColliderComponent> mCollider;

        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic;

        bool mIsKinematic {};
        bool mUseGravity {};
        bool mMass {};

    };

    using Rigidbody = RigidbodyComponent;
}
