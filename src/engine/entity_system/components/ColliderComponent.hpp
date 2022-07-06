#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/PhysicsTypes.hpp"

#include <foundation/PxTransform.h>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

// TODO: Use this link
// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html

namespace physx
{
    class PxShape;
    class PxRigidActor;
    class PxActor;
    class PxRigidDynamic;
    class PxRigidStatic;
}

namespace MFA
{

    class TransformComponent;

    class ColliderComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            ColliderComponent,
            FamilyType::Collider,
            EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::ActivationChangeEvent,
            Component
        )

        explicit ColliderComponent();

        ~ColliderComponent() override;

        void Init() override;

        void Shutdown() override;

        [[nodiscard]]
        Physics::SharedHandle<physx::PxRigidDynamic> GetRigidDynamic() const;

        void OnActivationStatusChanged(bool isActive) override;

    protected:

        virtual void OnTransformChange();

        virtual physx::PxTransform ComputePxTransform();

        virtual void ComputeTransform(
            physx::PxTransform const & inTransform,
            glm::vec3 & outPosition,
            glm::quat & outRotation
        );
        
        std::weak_ptr<TransformComponent> mTransform;

        physx::PxRigidActor * mActor = nullptr;
        // Either one of this 2 are available
        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic = nullptr;
        Physics::SharedHandle<physx::PxRigidStatic> mRigidStatic = nullptr;

        Physics::SharedHandle<physx::PxShape> mShape = nullptr;
        
        SignalId mTransformChangeListenerId = -1;

        bool mIsDynamic = false;

    };

    using Collider = ColliderComponent;

}
