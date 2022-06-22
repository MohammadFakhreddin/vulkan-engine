#include "RigidbodyComponent.h"
#include "Collider.h"

#include "EntitySystem/TransformComponent.h"
#include "Common/Physics/IPhysicsWorld.h"
#include "Common/EntitySystem/IEntity.h"
#include "Common/System/ISystem.h"

#include "Utils/MatrixUtils.h"

#include "PxPhysicsAPI.h"

#include <glm/gtc/quaternion.hpp>

using namespace physx;

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::ProcessEvent(const EntityEvent &event)
{
    switch (event.type)
    {
    case EEntityEvent::Initialize:
        init();
        break;
    case EEntityEvent::Update:
        update();
        break;
    default:
        break;
    }
}

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::SetIsKinematic(const bool isKinematic)
{
    mIsKinematic = isKinematic;
}

//-------------------------------------------------------------------------------------------------

bool RigidbodyComponent::IsKinematic() const noexcept
{
    return mIsKinematic;
}

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::SetOrientation(const glm::quat & orientation) const
{
    NSO_ASSERT(mActor != nullptr);
    const PxQuat pxQuat = MatrixUtils::GlmToPhysx(orientation);
    mActor->setGlobalPose(PxTransform {mActor->getGlobalPose().p, pxQuat});
}

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::init()
{
    auto * physicsWorld = gEnv->pPhysicsWorld;
   
    mTransformComponent = m_pEntity->GetComponent<TransformComponent>();
    if (mTransformComponent == nullptr)
        return;

    const glm::vec3 position = mTransformComponent->GetPosition();

    auto * collider = m_pEntity->GetComponent<Collider>();
    if (collider == nullptr)
        return;

    physx::PxShape *shape = collider->GetShape();

    PxRigidDynamic *body = physicsWorld->GetPhysX()->createRigidDynamic(MatrixUtils::PhysXTransform(position));
    NSO_ASSERT(body != nullptr);
    body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, mIsKinematic);
    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    physicsWorld->GetScene()->addActor(*body);
    mActor = body;
    
    mActor->userData = m_pEntity;
}

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::update() const
{
    NSO_ASSERT(mActor != nullptr);
    NSO_ASSERT(mActor->getGlobalPose().isValid());

    if (mIsKinematic)
    {
        if (mTransformComponent == nullptr)
            return;
        
        const auto transformPosition = mTransformComponent->GetPosition();
        const PxVec3 physxTransform  = MatrixUtils::GlmToPhysx(transformPosition);
        const auto transformRotation = mTransformComponent->GetRotation();
        const PxQuat physxRotation = MatrixUtils::GlmToPhysx(transformRotation);
        mActor->setGlobalPose(PxTransform {physxTransform, physxRotation});
    }
    else
    {
        updateTransformFromActor();
    }
}

//-------------------------------------------------------------------------------------------------

void RigidbodyComponent::updateTransformFromActor() const
{
    const auto position = mActor->getGlobalPose().p;
    const auto rotation = mActor->getGlobalPose().q;
    // TODO MFA: We may need to add a lot more stuff like velocity and many more stuff
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (transform == nullptr)
        return;

    transform->SetPosition(MatrixUtils::PhysXToGlm(position));
    transform->SetRotation(MatrixUtils::PhysXToGlm(rotation));
}

//-------------------------------------------------------------------------------------------------

glm::vec3 RigidbodyComponent::GetVelocity() const
{
    const PxVec3 pxVelocity = static_cast<PxRigidDynamic *>(mActor)->getLinearVelocity();
    return MatrixUtils::PhysXToGlm(pxVelocity);
}

void RigidbodyComponent::SetVelocity(glm::vec3 velocity)
{
    const PxVec3 pxVelocity = MatrixUtils::GlmToPhysx(velocity);

    static_cast<PxRigidDynamic *>(mActor)->setLinearVelocity(pxVelocity);
}

//-------------------------------------------------------------------------------------------------

float RigidbodyComponent::GetAngularDrag() const noexcept
{
    return static_cast<PxRigidDynamic *>(mActor)->getAngularDamping();
}

void RigidbodyComponent::SetAngularDrag(const float angularDrag)
{
    static_cast<PxRigidDynamic *>(mActor)->setAngularDamping(angularDrag);
}

//-------------------------------------------------------------------------------------------------
