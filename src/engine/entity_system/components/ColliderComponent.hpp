#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/PhysicsTypes.hpp"

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <physx/geometry/PxGeometry.h>

// TODO: Use this link
// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html

namespace physx
{
    class PxMaterial;
    class PxShape;
    class PxRigidActor;
    class PxActor;
    class PxRigidDynamic;
    class PxRigidStatic;
}

namespace MFA
{
    // TODO: Layer and layerMask
    class TransformComponent;

    class ColliderComponent : public Component
    {
    public:

        MFA_ABSTRACT_COMPONENT_PROPS(
            ColliderComponent,
            FamilyType::Collider,
            EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::ActivationChangeEvent,
            Component
        )

        explicit ColliderComponent(
            glm::vec3 const & center,
            Physics::SharedHandle<physx::PxMaterial> material
        );

        ~ColliderComponent() override;

        void Init() override;

        void Shutdown() override;

        void OnUI() override;

        [[nodiscard]]
        Physics::SharedHandle<physx::PxRigidDynamic> GetRigidDynamic() const;

        void OnActivationStatusChanged(bool isActive) override;

    protected:

        virtual void OnTransformChange();

        void CreateShape();

        void UpdateShapeCenter() const;

        void UpdateShapeGeometry();

        [[nodiscard]]
        virtual std::shared_ptr<physx::PxGeometry> ComputeGeometry() = 0;

        std::weak_ptr<TransformComponent> mTransform;

        physx::PxRigidActor * mActor = nullptr;
        // Either one of this 2 are available
        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic = nullptr;
        Physics::SharedHandle<physx::PxRigidStatic> mRigidStatic = nullptr;

        Physics::SharedHandle<physx::PxShape> mShape = nullptr;
        
        SignalId mTransformChangeListenerId = -1;

        bool mIsDynamic = false;

        glm::vec3 mCenter {};

        Physics::SharedHandle<physx::PxMaterial> mMaterial = nullptr;

        // Global world scale
        glm::vec3 mScale {};

    };

    using Collider = ColliderComponent;

}
