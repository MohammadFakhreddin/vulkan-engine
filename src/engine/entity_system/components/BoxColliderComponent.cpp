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
        glm::vec3 center = mCenter;

        UI::InputFloat3("Size", size);
        UI::InputFloat3("Center", center);

        if (Matrix::IsEqual(size, mHalfSize * 2.0f) == false)
        {
            mHalfSize = size * 0.5f;
            CreateShape();
        }

        if (Matrix::IsEqual(center, mCenter) == false)
        {
            mCenter = glm::vec4 {center.x, center.y, center.z, 1.0f};
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

    return Matrix::CopyGlmToPhysx(cubePosition, worldRotation);
}

//-------------------------------------------------------------------------------------------------

void MFA::BoxColliderComponent::CreateShape()
{
    mShape = Physics::CreateShape(
        *mActor,
        physx::PxBoxGeometry {mHalfSize.x, mHalfSize.y, mHalfSize.z},
        Physics::GetDefaultMaterial()->Ref()
    );
    MFA_ASSERT(mShape != nullptr);
    mShape->Ptr()->userData = this;
}

//-------------------------------------------------------------------------------------------------
