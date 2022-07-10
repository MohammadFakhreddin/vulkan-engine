#include "ColliderComponent.hpp"

#include "RigidbodyComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/physics/Physics.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/ui_system/UI_System.hpp"

#include <physx/PxRigidActor.h>
#include <physx/PxRigidStatic.h>
#include <physx/PxRigidDynamic.h>

namespace MFA
{

    using namespace physx;

    //-------------------------------------------------------------------------------------------------

    ColliderComponent::ColliderComponent(glm::vec3 const & center)
        : mCenter(center)
    {}

    //-------------------------------------------------------------------------------------------------

    ColliderComponent::~ColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Init()
    {
        Component::Init();

        mTransform = GetEntity()->GetComponent<Transform>();
        auto const transformComp = mTransform.lock();
        MFA_ASSERT(transformComp != nullptr);
        mTransformChangeListenerId = transformComp->RegisterChangeListener([this]()->void{
            OnTransformChange();
        });

        PxTransform pxTransform {};
        Copy(pxTransform.p, transformComp->GetWorldPosition());
        Copy(pxTransform.q, transformComp->GetWorldRotation());

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

        mScale = transformComp->GetWorldScale();

        CreateShape();
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

    void ColliderComponent::OnUI()
    {
        Parent::OnUI();
        if (UI::TreeNode(getName()))
        {
            if (UI::InputFloat<3>("Center", mCenter))
            {
                UpdateShapeCenter();
            }
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    Physics::SharedHandle<PxRigidDynamic> ColliderComponent::GetRigidDynamic() const
    {
        return mRigidDynamic;
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnActivationStatusChanged(bool const isActive)
    {
        Component::OnActivationStatusChanged(isActive);

        mActor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !isActive);
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnTransformChange()
    {
        auto const transformComp = mTransform.lock();
        MFA_ASSERT(transformComp != nullptr);

        auto const oldTransform = mActor->getGlobalPose();

        auto const & wPos = transformComp->GetWorldPosition();
        auto const & wRot = transformComp->GetWorldRotation();

        if (IsEqual(wPos, oldTransform.p) == false || IsEqual(wRot, oldTransform.q) == false)
        {
            PxTransform newTransform {};
            Copy(newTransform.p, transformComp->GetWorldPosition());
            Copy(newTransform.q, transformComp->GetWorldRotation());

            mActor->setGlobalPose(newTransform);
        }

        auto const & wScale = transformComp->GetWorldScale();
        if (IsEqual(wScale, mScale) == false)
        {
            mScale = wScale;

            UpdateShapeGeometry();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::CreateShape()
    {
        MFA_ASSERT(mShape == nullptr);

        // Note geometry can be null at this point
        auto const geometry = ComputeGeometry();

        mShape = Physics::CreateShape(
            *mActor,
            *geometry,
            Physics::GetDefaultMaterial()->Ref()
        );

        mShape->Ptr()->userData = this;

        UpdateShapeCenter();

        mActor->attachShape(mShape->Ref());
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::UpdateShapeCenter() const
    {
        PxTransform newTransform {};
        Copy(newTransform.p, mCenter);
        mShape->Ptr()->setLocalPose(newTransform);
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::UpdateShapeGeometry()
    {
        auto const geometry = ComputeGeometry();
        if (geometry != nullptr)    // It can be null because of geometry not being ready yet
        {
            mShape->Ptr()->setGeometry(*geometry);
        }
    }

    //-------------------------------------------------------------------------------------------------

    //PxTransform ColliderComponent::ComputePxTransform() const
    //{
    //    auto const transform = mTransform.lock();
    //    MFA_ASSERT(transform != nullptr);

    //    auto worldPosition = transform->GetWorldPosition();
    //    auto const & worldRotation = transform->GetWorldRotation();

    //    if (mHasNonZeroCenter)
    //    {
    //        worldPosition = worldPosition + glm::toMat4(transform->GetLocalRotationQuaternion()) * mCenter;
    //    }

    //    PxTransform pxTransform {};
    //    Copy(pxTransform.p, worldPosition);
    //    Copy(pxTransform.q, worldRotation);

    //    return pxTransform;
    //}
    //
    ////-------------------------------------------------------------------------------------------------

    //void ColliderComponent::ComputeTransform(
    //    PxTransform const & inTransform,
    //    glm::vec3 & outPosition,
    //    glm::quat & outRotation
    //) const
    //{
    //    Copy(outRotation, inTransform.q);
    //    
    //    if (mHasNonZeroCenter == false)
    //    {
    //        Copy(outPosition, inTransform.p);
    //    } else
    //    {
    //        auto const transform = mTransform.lock();
    //        MFA_ASSERT(transform != nullptr);
    //        
    //        auto const myWorldPos = Copy<glm::vec3, PxVec3>(inTransform.p);
    //        auto const myLocalPos = Copy<glm::vec3, glm::vec4>(glm::toMat4(transform->GetLocalRotationQuaternion()) * mCenter);

    //        outPosition = myWorldPos - myLocalPos;
    //    }
    //}

    //-------------------------------------------------------------------------------------------------

}
