#include "RigidbodyComponent.hpp"

#include "ColliderComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "TransformComponent.hpp"

#include <physx/PxRigidDynamic.h>

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    RigidbodyComponent::RigidbodyComponent(
        bool const isKinematic,
        bool const useGravity,
        bool const mass
    )
        : mIsKinematic(isKinematic)
        , mUseGravity(useGravity)
        , mMass(mass)
    {}

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::LateInit()
    {
        Component::LateInit();

        mTransform = GetEntity()->GetComponent<Transform>();
        MFA_ASSERT(mTransform.expired() == false);

        mCollider = GetEntity()->GetComponent<Collider>();
        auto const collider = mCollider.lock();
        MFA_ASSERT(collider != nullptr);

        mRigidDynamic = collider->GetRigidDynamic();
        MFA_ASSERT(mRigidDynamic != nullptr);

        UpdateMass();
        UpdateKinematic();
        UpdateGravity();
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::Update(float const deltaTimeInSec)
    {
        Component::Update(deltaTimeInSec);

        auto const transform = mTransform.lock();
        auto const collider = mCollider.lock();
        if (transform != nullptr && collider != nullptr)
        {
            auto const pxTransform = mRigidDynamic->Ptr()->getGlobalPose();

            glm::vec3 worldPosition;
            glm::quat worldRotation;

            collider->ComputeTransform(
                pxTransform,
                worldPosition,
                worldRotation
            );

            transform->UpdateWorldTransform(worldPosition, worldRotation);
            
        }
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::OnUI()
    {
        Component::OnUI();

        if (UI::Checkbox("Use gravity", mUseGravity))
        {
            UpdateGravity();
        }

        if (UI::Checkbox("Is kinematic", mIsKinematic))
        {
            UpdateKinematic();
        }

        if (UI::Checkbox("Mass", mMass))
        {
            UpdateMass();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::UpdateGravity() const
    {
        mRigidDynamic->Ptr()->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, mUseGravity);
    }
    
    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::UpdateKinematic() const
    {
        mRigidDynamic->Ptr()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, mIsKinematic);
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::UpdateMass() const
    {
        mRigidDynamic->Ptr()->setMass(mMass);
    }

    //-------------------------------------------------------------------------------------------------

}
