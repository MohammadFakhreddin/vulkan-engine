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

        // TODO: What if rigid body is inside parent ?
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

    void ColliderComponent::OnTransformChange()
    {
        if (mTransformChangeBySelf == true) // In case the rigid body has caused the change
        {
            mTransformChangeBySelf = false;
            return;
        }

        mActor->setGlobalPose(ComputePxTransform());
    }

    //-------------------------------------------------------------------------------------------------

    physx::PxTransform ColliderComponent::ComputePxTransform()
    {
        auto const transform = mTransform.lock();
        MFA_ASSERT(transform != nullptr);

        auto const & worldPosition = transform->GetWorldPosition();
        auto const & worldRotation = transform->GetWorldRotation();

        return Matrix::CopyGlmToPhysx(worldPosition, worldRotation);
    }

    //-------------------------------------------------------------------------------------------------

}
