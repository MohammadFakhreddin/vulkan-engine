#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/Physics.hpp"

namespace MFA
{
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

    private:

        void UpdateGravity() const;
        void UpdateKinematic() const;
        void UpdateMas() const;

        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic;

        bool mIsKinematic {};
        bool mUseGravity {};
        bool mMas {};

    };

    using Rigidbody = RigidbodyComponent;
}
