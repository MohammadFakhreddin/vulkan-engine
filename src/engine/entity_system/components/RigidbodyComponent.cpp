#include "RigidbodyComponent.hpp"

#include "ColliderComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "TransformComponent.hpp"

#include <physx/PxRigidDynamic.h>

namespace MFA
{

    using namespace physx;

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

            MFA_LOG_INFO(
                "Rigidbody\n Position: %f %f %f\n Rotation: %f %f %f %f"
                , position.x, position.y, position.z
                , rotation.x, rotation.y, rotation.z, rotation.w
            );

            transform->SetWorldTransform(position, rotation);
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

            if (UI::InputFloat("Mass", mMass))
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

    void RigidbodyComponent::SetLinearVelocity(glm::vec3 const & velocity) const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        mRigidDynamic->Ptr()->setLinearVelocity(Copy<PxVec3>(velocity));
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 RigidbodyComponent::GetLinearVelocity() const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        auto const velocity = mRigidDynamic->Ptr()->getLinearVelocity();
        return Copy<glm::vec3>(velocity);
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::SetAngularVelocity(glm::vec3 const & angularVelocity) const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        mRigidDynamic->Ptr()->setAngularVelocity(Copy<PxVec3>(angularVelocity));
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 RigidbodyComponent::GetAngularVelocity() const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        return Copy<glm::vec3>(mRigidDynamic->Ptr()->getAngularVelocity());
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::AddForce(glm::vec3 const & force) const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        mRigidDynamic->Ptr()->addForce(Copy<PxVec3>(force));
    }

    //-------------------------------------------------------------------------------------------------

    void RigidbodyComponent::AddTorque(glm::vec3 const & torque) const
    {
        MFA_ASSERT(mRigidDynamic != nullptr);
        mRigidDynamic->Ptr()->addTorque(Copy<PxVec3>(torque));
    }

    //-------------------------------------------------------------------------------------------------

    // TODO: setForceAndTorque

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
