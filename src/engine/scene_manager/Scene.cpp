#include "./Scene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void Scene::Init()
    {
        mRootEntity = EntitySystem::CreateEntity("SceneRoot", nullptr);

        prepareCameraBuffer();
        preparePointLightsBuffer();
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::OnPreRender(float deltaTimeInSec, RT::CommandRecordState & recordState)
    {
        if (auto const cameraPtr = mActiveCamera.lock())
        {
            if (cameraPtr->IsCameraDataDirty())
            {
                RF::UpdateUniformBuffer(
                    mCameraBufferCollection.buffers[recordState.frameIndex],
                    CBlobAliasOf(cameraPtr->GetCameraData())
                );
            }
        }

        // Maybe we can search for another active camera 
        mPointLightData.count = 0;
        for (int i = static_cast<int>(mPointLights.size()) - 1; i >= 0; --i)
        {
            if (mPointLights[i].expired())
            {
                mPointLights[i] = mPointLights.back();
                mPointLights.pop_back();
            }
        }
        for (auto & mPointLight : mPointLights)
        {
            if (auto const ptr = mPointLight.lock())
            {
                if (ptr->IsVisible())
                {
                    MFA_ASSERT(mPointLightData.count < MAX_VISIBLE_POINT_LIGHT_COUNT);
                    auto & item = mPointLightData.items[mPointLightData.count];

                    //Future optimization: if (item.id != ptr->GetUniqueId() || ptr->IsDataDirty() == true){
                    Matrix::CopyGlmToCells(ptr->GetLightColor(), item.color);
                    Matrix::CopyGlmToCells(ptr->GetPosition(), item.position);
                    ptr->GetShadowViewProjectionMatrices(item.viewProjectionMatrices);
                    item.maxSquareDistance = ptr->GetMaxSquareDistance();
                    item.linearAttenuation = ptr->GetLinearAttenuation();
                    item.quadraticAttenuation = ptr->GetQuadraticAttenuation();
                    
                    ++mPointLightData.count;
                }
            }
        }

        RF::UpdateUniformBuffer(
            recordState,
            mPointLightsBufferCollection,
            CBlobAliasOf(mPointLightData)
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Shutdown()
    {
        auto const deleteResult = EntitySystem::DestroyEntity(mRootEntity);
        MFA_ASSERT(deleteResult == true);

        RF::DestroyUniformBuffer(mCameraBufferCollection);
        RF::DestroyUniformBuffer(mPointLightsBufferCollection);
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

    RT::UniformBufferCollection const & Scene::GetCameraBufferCollection() const
    {
        return mCameraBufferCollection;
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight)
    {
        mPointLights.emplace_back(pointLight);
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferCollection const & Scene::GetPointLightsBufferCollection() const
    {
        return mPointLightsBufferCollection;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t Scene::GetPointLightCount() const
    {
        return mPointLightData.count;
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::prepareCameraBuffer()
    {
        mCameraBufferCollection = RF::CreateUniformBuffer(
            sizeof(CameraComponent::CameraBufferData),
            RF::GetMaxFramesPerFlight()
        );

        CameraComponent::CameraBufferData const cameraBufferData{};

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                mCameraBufferCollection.buffers[frameIndex],
                CBlobAliasOf(cameraBufferData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::preparePointLightsBuffer()
    {
        mPointLightsBufferCollection = RT::UniformBufferCollection(RF::CreateUniformBuffer(
            sizeof(PointLightsBufferData),
            RF::GetMaxFramesPerFlight()
        ));

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                mPointLightsBufferCollection.buffers[frameIndex],
                CBlobAliasOf(mPointLightData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
