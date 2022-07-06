#include "RigidbodyComponent.hpp"

#include <physx/PxRigidDynamic.h>

#include "ColliderComponent.hpp"
#include "engine/entity_system/Entity.hpp"

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
        , mMas(mass)
    {}

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::LateInit()
    {
        Component::LateInit();

        auto const collider = GetEntity()->GetComponent<Collider>().lock();
        MFA_ASSERT(collider != nullptr);
        mRigidDynamic = collider->GetRigidDynamic();
        MFA_ASSERT(mRigidDynamic != nullptr);

        UpdateMas();
        UpdateKinematic();
        UpdateGravity();
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::Update(float const deltaTimeInSec)
    {
        Component::Update(deltaTimeInSec);
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::OnUI()
    {
        Component::OnUI();

        // TODO
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

    void RigidbodyComponent::UpdateMas() const
    {
        mRigidDynamic->Ptr()->setMass(mMas);
    }

    //-------------------------------------------------------------------------------------------------

}
