#include "./Scene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/entity_system/EntitySystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void Scene::OnPreRender(float deltaTimeInSec, RT::CommandRecordState & recordState)
    {
        if (auto const cameraPtr = mActiveCamera.lock())
        {
            if (cameraPtr->IsCameraDataDirty())
            {
                RF::UpdateUniformBuffer(
                    mCameraBufferCollection->buffers[recordState.frameIndex],
                    CBlobAliasOf(cameraPtr->GetCameraData())
                );
            }
        }

        // Maybe we can search for active camera if nothing was found
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Init()
    {
        mRootEntity = EntitySystem::CreateEntity("SceneRoot", nullptr);

        mCameraBufferCollection = std::make_unique<RT::UniformBufferCollection>(RF::CreateUniformBuffer(
            sizeof(CameraComponent::CameraBufferData),
            RF::GetMaxFramesPerFlight()
        ));

        CameraComponent::CameraBufferData const cameraBufferData {};

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                mCameraBufferCollection->buffers[frameIndex],
                CBlobAliasOf(cameraBufferData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Shutdown()
    {
        auto const deleteResult = EntitySystem::DestroyEntity(mRootEntity);
        MFA_ASSERT(deleteResult == true);

        RF::DestroyUniformBuffer(*mCameraBufferCollection);
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

    RT::UniformBufferCollection * Scene::GetCameraBufferCollection() const
    {
        return mCameraBufferCollection.get();
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight)
    {
        mPointLights.emplace_back(pointLight);
    }

    //-------------------------------------------------------------------------------------------------
}
