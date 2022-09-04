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

        explicit RigidbodyComponent();

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
        void UpdateKinematicRotation() const;

        std::weak_ptr<TransformComponent> mTransform;

        std::weak_ptr<ColliderComponent> mCollider;

        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic;

        MFA_ATOMIC_VARIABLE2(IsKinematic, bool, false, UpdateKinematic)

        MFA_ATOMIC_VARIABLE2(UseGravity, bool, true, UpdateGravity)

        MFA_ATOMIC_VARIABLE2(Mass, float, 1.0f, UpdateMass)

        MFA_ATOMIC_VARIABLE2(KinematicRotation, bool, true, UpdateKinematicRotation)

        MFA_ATOMIC_VARIABLE1(UpdateTransformPosition, bool, true)

        MFA_ATOMIC_VARIABLE1(UpdateTransformRotation, bool, true)
    };

    using Rigidbody = RigidbodyComponent;
}
