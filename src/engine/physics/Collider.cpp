#include "Collider.h"

#include "IEntity.h"
#include "RigidbodyComponent.h"
#include "EntitySystem/TransformComponent.h"
#include "Physics/IPhysicsWorld.h"
#include "System/ISystem.h"
#include "Utils/MatrixUtils.h"
#include "System/ITimer.h"

#include <PxShape.h>
#include <PxFiltering.h>
#include <PxRigidStatic.h>
#include <PxPhysics.h>
#include <PxScene.h>

//-------------------------------------------------------------------------------------------------

void Collider::ProcessEvent(const EntityEvent & event)
{
    switch (event.type)
    {
    case EEntityEvent::Initialize:
        baseInit();
        break;
    case EEntityEvent::PhysicsCollision:
    {
        auto * collision = reinterpret_cast<Collision *>(event.nParam[0]);
        NSO_ASSERT(collision != nullptr);
        handleCollisionEvent(*collision);
        break;
    }
    case EEntityEvent::Update:
    {
        update(ITimer::SIMULATION_SEC);
        break;
    }
    case EEntityEvent::Remove:
    {
        shutdown();
        break;
    }
    default:
        break;
    }
}

//-------------------------------------------------------------------------------------------------

void Collider::SetLayerMask(const LayerMask & layerMask)
{
    mLayerMask = layerMask;
}

//-------------------------------------------------------------------------------------------------

const LayerMask & Collider::GetLayerMask() const noexcept
{
    return mLayerMask;
}

//-------------------------------------------------------------------------------------------------

uint16_t Collider::RegisterCollisionListener(ICollisionListener * classPtr)
{
    NSO_ASSERT(classPtr != nullptr);
    const auto listenerId = mNextListenerId++;
    mCollisionListeners.emplace_back(Listener{ listenerId, classPtr });
    return listenerId;
}

//-------------------------------------------------------------------------------------------------

void Collider::RemoveCollisionListener(const uint16_t id)
{
    for (int i = static_cast<int>(mCollisionListeners.size()) - 1; i >= 0; --i)
    {
        if (mCollisionListeners[i].id == id)
        {
            mCollisionListeners.erase(mCollisionListeners.begin() + i);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void Collider::baseInit()
{
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (transform != nullptr)
    {
        // TODO: Is this a right thing to do ?
        transform->SetPosition(transform->GetPosition() + mCenter);
    }

    const auto checkForStaticRigidActor = init();

    NSO_ASSERT(mShape != nullptr);

    // Assigning LayerMask to shapeQueryFilterData
    auto queryFilterData = mShape->getQueryFilterData();
    queryFilterData.word0 = mLayerMask.GetValue();
    mShape->setQueryFilterData(queryFilterData);

    if (checkForStaticRigidActor)
    {
        mAttachedRigidbody = m_pEntity->GetComponent<RigidbodyComponent>();
        // The RigidbodyComponent will create dynamic physics body for this collider.
        if (mAttachedRigidbody != nullptr)
        {
            return;
        }

        // For static objects (No rigid-body) the collider automatically creates static rigid-body

        auto * physicsWorld = gEnv->pPhysicsWorld;
        NSO_ASSERT(physicsWorld != nullptr);

        if (!NSO_VERIFY(transform != nullptr))
            return;

        const auto position = transform->GetPosition();
        const auto rotation = transform->GetRotation();

        mStaticRigidActor = physicsWorld->GetPhysX()->createRigidStatic(MatrixUtils::PhysXTransform(position, rotation));
        NSO_ASSERT(mStaticRigidActor != nullptr);
        mStaticRigidActor->attachShape(*mShape);
        physicsWorld->GetScene()->addActor(*mStaticRigidActor);

        mStaticRigidActor->userData = m_pEntity;
    }
}

//-------------------------------------------------------------------------------------------------

void Collider::handleCollisionEvent(const Collision & collision)
{
    if (mCollisionListeners.empty() == false)
    {
        for (const auto & listener : mCollisionListeners)
        {
            NSO_ASSERT(listener.classPtr != nullptr);
            listener.classPtr->onCollision(collision);
        }
    }
}
