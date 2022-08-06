#include "SphereCollider.h"

#include "IEntity.h"
#include "Common/System/ISystem.h"
#include "Common/Physics/IPhysicsWorld.h"
#include "PxPhysicsAPI.h"
#include "EntitySystem/TransformComponent.h"

using namespace physx;

bool SphereCollider::init()
{
    auto *physicsWorld = gEnv->pPhysicsWorld;
    mMaterial = physicsWorld->GetPhysX()->createMaterial(0.5f, 0.5f, 0.6f);

    auto geometryRadius = mRadius;
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (transform != nullptr)
    {
        // Applying transform scale on geometry
        geometryRadius *= transform->GetScale().x;
    }

    mShape = physicsWorld->GetPhysX()->createShape(PxSphereGeometry(geometryRadius), *mMaterial);

    mShape->userData = this;

    return true;
}
