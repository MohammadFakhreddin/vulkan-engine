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

            auto const position = Copy<glm::vec3>(pxTransform.p);
            auto const rotation = Copy<glm::quat>(pxTransform.q);

            transform->UpdateWorldTransform(position, rotation);
            
        }
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::OnUI()
    {
        if (UI::TreeNode("Rigid-body"))
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

            if (UI::InputFloat<1>("Mass", mMass))
            {
                UpdateMass();
            }

            UI::TreePop();
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::Clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::Serialize(nlohmann::json & jsonObject) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
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
