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

        void SetLinearVelocity(glm::vec3 const & velocity) const;

        [[nodiscard]]
        glm::vec3 GetLinearVelocity() const;

        void SetAngularVelocity(glm::vec3 const & angularVelocity) const;

        [[nodiscard]]
        glm::vec3 GetAngularVelocity() const;

        void AddForce(glm::vec3 const & force) const;

        void AddTorque(glm::vec3 const & torque) const;

    private:

        void UpdateGravity() const;
        void UpdateKinematic() const;
        void UpdateMass() const;

        std::weak_ptr<TransformComponent> mTransform;

        std::weak_ptr<ColliderComponent> mCollider;

        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic;

        bool mIsKinematic {};
        bool mUseGravity {};
        float mMass {};

    };

    using Rigidbody = RigidbodyComponent;
}
