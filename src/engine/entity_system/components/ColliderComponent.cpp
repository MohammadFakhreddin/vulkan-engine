#include "ColliderComponent.hpp"

#include "RigidbodyComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/physics/Physics.hpp"
#include "engine/BedrockMatrix.hpp"

#include <physx/PxRigidActor.h>
#include <physx/PxRigidStatic.h>
#include <physx/PxRigidDynamic.h>

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    ColliderComponent::ColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    ColliderComponent::~ColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Init()
    {
        Component::Init();

        mTransform = GetEntity()->GetComponent<Transform>();
        auto const transform = mTransform.lock();
        MFA_ASSERT(transform != nullptr);
        mTransformChangeListenerId = transform->RegisterChangeListener([this]()->void{
            OnTransformChange();
        });

        auto const pxTransform = ComputePxTransform();

        auto const rigidBody = GetEntity()->GetComponent<Rigidbody>().lock();
        if (rigidBody != nullptr)
        {

            mIsDynamic = true;
            mRigidDynamic = Physics::CreateDynamicActor(pxTransform);
            mActor = mRigidDynamic->Ptr();

        } else
        {

            mIsDynamic = false;
            mRigidStatic = Physics::CreateStaticActor(pxTransform);
            mActor = mRigidStatic->Ptr();
        }

        MFA_ASSERT(mActor != nullptr);
        mActor->userData = this;
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Shutdown()
    {
        Component::Shutdown();
        if (auto const transform = mTransform.lock())
        {
            transform->UnRegisterChangeListener(mTransformChangeListenerId);
        }
    }

    //-------------------------------------------------------------------------------------------------

    Physics::SharedHandle<physx::PxRigidDynamic> ColliderComponent::GetRigidDynamic() const
    {
        return mRigidDynamic;
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnActivationStatusChanged(bool const isActive)
    {
        Component::OnActivationStatusChanged(isActive);

        mActor->setActorFlag(physx::PxActorFlag::eDISABLE_SIMULATION, !isActive);
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnTransformChange()
    {
        // TODO: Look for a better solution
        auto const oldTransform = mActor->getGlobalPose();
        auto const newTransform = ComputePxTransform();
        if (newTransform.p == oldTransform.p && newTransform.q == oldTransform.q)
        {
            return;
        }
        mActor->setGlobalPose(newTransform);
    }

    //-------------------------------------------------------------------------------------------------

    physx::PxTransform ColliderComponent::ComputePxTransform()
    {
        auto const transform = mTransform.lock();
        MFA_ASSERT(transform != nullptr);

        auto const & worldPosition = transform->GetWorldPosition();
        auto const & worldRotation = transform->GetWorldRotation();

        physx::PxTransform pxTransform {};
        Copy(pxTransform.p, worldPosition);
        Copy(pxTransform.q, worldRotation);

        return pxTransform;
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::ComputeTransform(
        physx::PxTransform const & inTransform,
        glm::vec3 & outPosition,
        glm::quat & outRotation
    )
    {
        Copy(outPosition, inTransform.p);
        Copy(outRotation, inTransform.q);
    }

    //-------------------------------------------------------------------------------------------------

}
