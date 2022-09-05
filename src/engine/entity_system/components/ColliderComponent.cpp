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

    ColliderComponent::ColliderComponent() = default;

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
        
        auto const rigidBody = GetEntity()->GetComponent<Rigidbody>();
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

        MFA_ASSERT(mActor != nullptr);
        mActor->userData = this;

        mScale = transformComp->GetWorldScale();

        CreateShape();

        mTransformChangeListenerId = transformComp->RegisterChangeListener([this](Transform::ChangeParams const & params)->void{
            OnTransformChange(params);
        });

        Physics::AddActor(*mActor);

        mInitialized = true;
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::Shutdown()
    {
        Component::Shutdown();

        if (auto const transform = mTransform.lock())
        {
            transform->UnRegisterChangeListener(mTransformChangeListenerId);
        }

        RemoveShapes();

        if (mActor != nullptr)
        {
            Physics::RemoveActor(*mActor);
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnUI()
    {
        if (UI::TreeNode(Name))
        {
            Parent::OnUI();

            bool relativeTransformChanged = UI::InputFloat("Center", mCenter);
            auto eulerAngles = mRotation.GetEulerAngles();
            relativeTransformChanged |= UI::InputFloat("Euler angles", eulerAngles);

            if (relativeTransformChanged)
            {
                UpdateShapeRelativeTransform();
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
        mActor->setActorFlag(PxActorFlag::eVISUALIZATION, isActive);
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

    void ColliderComponent::OnTransformChange(Transform::ChangeParams const & params)
    {
        if (params.worldPositionChanged == false && params.worldRotationChanged == false)
        {
            return;
        }

        auto const transformComp = mTransform.lock();
        MFA_ASSERT(transformComp != nullptr);

        PxTransform newTransform = mActor->getGlobalPose();

        if (params.worldPositionChanged)
        {
            Copy(newTransform.p, transformComp->GetWorldPosition());
        }
        if (params.worldRotationChanged)
        {
            Copy(newTransform.q, transformComp->GetWorldRotation());
        }

        mActor->setGlobalPose(newTransform);
        
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
        MFA_ASSERT(mShapes.empty());

        // Note geometry can be null at this point
        auto const geometries = ComputeGeometry();

        CreateShape(geometries);
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::UpdateShapeRelativeTransform() const
    {
        for (auto & shape : mShapes)
        {
            // Updating the center without modifying rotation
            auto newTransform = shape->getLocalPose();
            Copy(newTransform.p, mCenter);
            Copy(newTransform.q, mRotation.GetQuaternion());
            shape->setLocalPose(newTransform);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::UpdateShapeGeometry()
    {
        auto const geometries = ComputeGeometry();
        if (geometries.empty())
        {
            return;
        }

        if (mShapes.size() == geometries.size())
        {
            for (size_t i = 0; i < geometries.size(); ++i)
            {
                mShapes[i]->setGeometry(*geometries[i]);
            }
        } else
        {
            RemoveShapes();
            CreateShape(geometries);
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::CreateShape(std::vector<std::shared_ptr<physx::PxGeometry>> const & geometries)
    {
        for (auto const & geometry : geometries)
        {
            mShapes.emplace_back();
            auto & shape = mShapes.back();
            shape = Physics::CreateShape(
                *mActor,
                *geometry,
                Physics::GetDefaultMaterial()->Ref()
            );
            MFA_ASSERT(shape != nullptr);
            MFA_ASSERT(shape->isExclusive() == true);

            shape->userData = this;
        }

        UpdateShapeRelativeTransform();
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::RemoveShapes()
    {
        for (auto const & shape : mShapes)
        {
            mActor->detachShape(*shape);
        }
        mShapes.clear();
    }

    //-------------------------------------------------------------------------------------------------

    void ColliderComponent::OnMaterialChange() const
    {
        std::vector<PxMaterial *> const materials { mMaterial->Ptr() };
        
        for (auto * shape : mShapes)
        {
            shape->setMaterials(materials.data(), static_cast<PxU16>(materials.size()));
        }
    }

    //-------------------------------------------------------------------------------------------------

}
