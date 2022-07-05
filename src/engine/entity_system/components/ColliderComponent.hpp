#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/PhysicsTypes.hpp"

#include <foundation/PxTransform.h>

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
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        explicit ColliderComponent();

        ~ColliderComponent() override;

        void Init() override;

        void Shutdown() override;

    protected:

        virtual void OnTransformChange();

        virtual physx::PxTransform ComputePxTransform();
        
        std::weak_ptr<TransformComponent> mTransform;

        physx::PxRigidActor * mActor = nullptr;
        // Either one of this 2 are available
        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic = nullptr;
        Physics::SharedHandle<physx::PxRigidStatic> mRigidStatic = nullptr;

        Physics::SharedHandle<physx::PxShape> mShape = nullptr;
        
        SignalId mTransformChangeListenerId = -1;

        bool mIsDynamic = false;

        bool mTransformChangeBySelf = false;

    };

    using Collider = ColliderComponent;

}
