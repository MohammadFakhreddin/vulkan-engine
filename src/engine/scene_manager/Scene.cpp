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
        if (mActiveCamera != nullptr && mActiveCamera->IsCameraDataDirty())
        {
            RF::UpdateUniformBuffer(
                mCameraBufferCollection->buffers[recordState.frameIndex],
                CBlobAliasOf(mActiveCamera->GetCameraData())
            );
        }
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

    RT::UniformBufferCollection * Scene::GetCameraBufferCollection() const
    {
        return mCameraBufferCollection.get();
    }

    //-------------------------------------------------------------------------------------------------
}
