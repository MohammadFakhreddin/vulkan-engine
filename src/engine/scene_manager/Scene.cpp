#include "./Scene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void Scene::Init()
    {
        mRootEntity = EntitySystem::CreateEntity("SceneRoot", nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Shutdown()
    {
        EntitySystem::DestroyEntity(mRootEntity);
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::SetActiveCamera(std::weak_ptr<CameraComponent> const & camera)
    {
        MFA_ASSERT(camera.expired() == false);
        mActiveCamera = camera;
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<CameraComponent> const & Scene::GetActiveCamera() const
    {
        return mActiveCamera;
    }

    //-------------------------------------------------------------------------------------------------

    Entity * Scene::GetRootEntity() const
    {
        MFA_ASSERT(mRootEntity != nullptr);
        return mRootEntity;
    }

    //-------------------------------------------------------------------------------------------------

}
