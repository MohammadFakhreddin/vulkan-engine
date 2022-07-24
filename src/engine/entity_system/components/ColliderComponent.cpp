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

    ColliderComponent::ColliderComponent(
        glm::vec3 const & center,
        Physics::SharedHandle<PxMaterial> material
    )
        : mCenter(center)
        , mMaterial(std::move(material))
    {}

    //-------------------------------------------------------------------------------------------------

    ColliderComponent::~ColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Init()
    {
        Parent::Init();

        if (mMaterial == nullptr)
        {
            mMaterial = Physics::GetDefaultMaterial();
        }

        mTransform = GetEntity()->GetComponent<Transform>();
        auto const transformComp = mTransform.lock();
        MFA_ASSERT(transformComp != nullptr);
        
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

        mTransformChangeListenerId = transformComp->RegisterChangeListener([this]()->void{
            OnTransformChange();
        });

        Physics::AddActor(*mActor);
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Shutdown()
    {
        Component::Shutdown();
        if (auto const transform = mTransform.lock())
        {
            transform->UnRegisterChangeListener(mTransformChangeListenerId);
        }
        Physics::RemoveActor(*mActor);
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnUI()
    {
        if (UI::TreeNode(Name))
        {
            Parent::OnUI();
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

    void ColliderComponent::Serialize(nlohmann::json & jsonObject) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
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
        MFA_ASSERT(mShape->Ptr()->isExclusive() == true);

        mShape->Ptr()->userData = this;

        UpdateShapeCenter();
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::UpdateShapeCenter() const
    {
        PxTransform newTransform {};
        Copy(newTransform.p, mCenter);
        Copy(newTransform.q, glm::identity<glm::quat>());
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

}
