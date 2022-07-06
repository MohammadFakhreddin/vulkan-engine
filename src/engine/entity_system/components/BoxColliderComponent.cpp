#include "BoxColliderComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/physics/Physics.hpp"
#include "engine/ui_system/UI_System.hpp"

#include <physx/PxRigidActor.h>

//-------------------------------------------------------------------------------------------------

MFA::BoxColliderComponent::BoxColliderComponent(
    glm::vec3 const size,
    glm::vec3 const center
)
    : mHalfSize(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f)
    , mCenter(center.x, center.y, center.z, 1.0f)
{}

//-------------------------------------------------------------------------------------------------

MFA::BoxColliderComponent::~BoxColliderComponent() = default;

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::Init()
{
    ColliderComponent::Init();

    MFA_ASSERT(mActor != nullptr);

    CreateShape();

    mActor->attachShape(mShape->Ref());
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::Shutdown()
{
    ColliderComponent::Shutdown();
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::OnUI()
{
    if (UI::TreeNode("BoxCollider"))
    {
        glm::vec3 size = mHalfSize * 2.0f;
        
        if (UI::InputFloat<3>("Size", size))
        {
            mHalfSize = size * 0.5f;
            CreateShape();
        }

        if (UI::InputFloat<3>("Center", mCenter))
        {
            ComputePxTransform();
        }

        ColliderComponent::OnUI();
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::OnTransformChange()
{
    ColliderComponent::OnTransformChange();
}

//-------------------------------------------------------------------------------------------------

physx::PxTransform MFA::BoxColliderComponent::ComputePxTransform()
{
    auto const transform = mTransform.lock();
    MFA_ASSERT(transform != nullptr);

    auto const & worldPosition = transform->GetWorldPosition();
    auto const & worldRotation = transform->GetWorldRotation();

    auto const cubePosition = worldPosition + glm::toMat4(worldRotation) * mCenter;

    physx::PxTransform pxTransform {};
    Copy(pxTransform.p, worldPosition);
    Copy(pxTransform.q, worldRotation);
    return pxTransform;
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::ComputeTransform(physx::PxTransform const & pxTransform, glm::vec3 & outPosition, glm::quat & outRotation)
{
    ColliderComponent::ComputeTransform(pxTransform, outPosition, outRotation);
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::CreateShape()
{
    auto const transform = mTransform.lock();
    MFA_ASSERT(transform != nullptr);

    auto const scale = transform->GetWorldScale();

    mShape = Physics::CreateShape(
        *mActor,
        physx::PxBoxGeometry {mHalfSize.x * scale.x, mHalfSize.y * scale.y, mHalfSize.z * scale.z},
        Physics::GetDefaultMaterial()->Ref()
    );
    MFA_ASSERT(mShape != nullptr);
    mShape->Ptr()->userData = this;
}

//-------------------------------------------------------------------------------------------------
