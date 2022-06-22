#include "CapsuleCollider.h"


#include "IEntity.h"
#include "System/ISystem.h"
#include "Physics/IPhysicsWorld.h"
#include "PxPhysicsAPI.h"
#include "EntitySystem/TransformComponent.h"
#include "Utils/MatrixUtils.h"

using namespace physx;

bool CapsuleCollider::init()
{
    auto * physicsWorld = gEnv->pPhysicsWorld;
    mMaterial = physicsWorld->GetPhysX()->createMaterial(0.5f, 0.5f, 0.6f);

    auto geometryRadius = mRadius;
    auto geometryHeight = mHeight;
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (transform != nullptr)
    {
        // Applying transform scale on geometry
        geometryRadius *= transform->GetScale().x;
        geometryHeight *= transform->GetScale().z;
    }

    mShape = physicsWorld->GetPhysX()->createShape(
        PxCapsuleGeometry(
            geometryRadius,
            geometryHeight / 2.0f
        ),
        *mMaterial
    );

    mShape->setLocalPose(MatrixUtils::PhysXTransform(mDirection));

    mShape->userData = this;

    return true;
}
