#include "./Scene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/EntitySystem.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

void Scene::Init()
{
    mRootEntity = EntitySystem::CreateEntity("SceneRoot", nullptr);
}

//-------------------------------------------------------------------------------------------------

void Scene::Shutdown()
{
    auto const deleteResult = EntitySystem::DestroyEntity(mRootEntity);
    MFA_ASSERT(deleteResult == true);
}

//-------------------------------------------------------------------------------------------------

void Scene::SetActiveCamera(CameraComponent * camera)
{
    MFA_ASSERT(camera != nullptr);
    mActiveCamera = camera;
}

//-------------------------------------------------------------------------------------------------

CameraComponent * Scene::GetActiveCamera() const
{
    MFA_ASSERT(mActiveCamera != nullptr);
    return mActiveCamera;
}

//-------------------------------------------------------------------------------------------------

Entity * Scene::GetRootEntity() const
{
    MFA_ASSERT(mRootEntity != nullptr);
    return mRootEntity;
}

//-------------------------------------------------------------------------------------------------

