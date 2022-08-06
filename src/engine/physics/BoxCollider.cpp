#include "BoxCollider.h"

#include "IEntity.h"
#include "Common/System/ISystem.h"
#include "EntitySystem/TransformComponent.h"

#include "Common/Physics/IPhysicsWorld.h"
#include "PxPhysicsAPI.h"

#include "ThirdParty/nlohmann/json.hpp"

using namespace physx;
using namespace FileLoaderType;

//-------------------------------------------------------------------------------------------------

void BoxCollider::Deserialize(const JsonObject &jsonObject)
{
    int version = jsonObject["SerializedVersion"].get<int>();
    if (version != SerializedVersion)
    {
        // Hanle versioning.
        return;
    }

    mCenter = deserializeCenter(jsonObject);
    mSize = deserializeSize(jsonObject);
}

//-------------------------------------------------------------------------------------------------

bool BoxCollider::init()
{
    auto * physicsWorld = gEnv->pPhysicsWorld;
    NSO_ASSERT(physicsWorld != nullptr);

    mMaterial = physicsWorld->GetPhysX()->createMaterial(0.5f, 0.5f, 0.6f);
    NSO_ASSERT(mMaterial != nullptr);

    glm::vec3 geometrySize = mSize;
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (transform != nullptr)
    {
        // Applying transform scale on geometry
        geometrySize *= transform->GetScale();
    }

    mShape = physicsWorld->GetPhysX()->createShape(PxBoxGeometry(geometrySize.x / 2.0f, geometrySize.y / 2.0f, geometrySize.z / 2.0f), *mMaterial);
    NSO_ASSERT(mShape != nullptr);

    mShape->userData = this;

    RegisterCollisionListener(this);

    return true;
}

//-------------------------------------------------------------------------------------------------

void BoxCollider::onCollision(const Collision & collision)
{
    /*
    printf(
        "\n-------------------------------\n"
        "BoxCollision:\n"
        "EntityId: %d\n"
        "OtherEntity: %d\n"
        "ContactPointsCount: %llu\n"
        "ContactPoint0:\n"
        "PointX %f pointY %f pointZ %f,\n"
        "ColliderEntity: %d, OtherColliderEntity: %d\n"
        "-------------------------------\n"
        , m_pEntity->GetHandle()
        , collision.otherEntity->GetHandle()
        , collision.contactPoints.size()
        , collision.contactPoints[0].point.x
        , collision.contactPoints[0].point.y
        , collision.contactPoints[0].point.z
        , collision.contactPoints[0].thisCollider->GetEntityHandle()
        , collision.contactPoints[0].otherCollider->GetEntityHandle()
    );
    */
}

//-------------------------------------------------------------------------------------------------

glm::vec3 BoxCollider::deserializeSize(const JsonObject &jsonObject)
{
    const auto & rawSize = jsonObject["Size"];

    glm::vec3 size {};

    size.x = rawSize["x"].get<float>();
    size.y = rawSize["y"].get<float>();
    size.z = rawSize["z"].get<float>();

    return size;
}

//-------------------------------------------------------------------------------------------------

glm::vec3 BoxCollider::deserializeCenter(const JsonObject &jsonObject)
{
    const auto & rawCenter = jsonObject["Center"];

    glm::vec3 center {};

    center.x = rawCenter["x"].get<float>();
    center.y = rawCenter["y"].get<float>();
    center.z = rawCenter["z"].get<float>();

    return center;
}

//-------------------------------------------------------------------------------------------------
