#include "./Scene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void Scene::OnResize()
    {
        for (auto * pipeline : mActivePipelines)
        {
            pipeline->OnResize();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Init()
    {
        mRootEntity = EntitySystem::CreateEntity("SceneRoot", nullptr);

        prepareCameraBuffer();
        prepareDirectionalLightsBuffer();
        preparePointLightsBuffer();
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::OnPreRender(float deltaTimeInSec, RT::CommandRecordState & recordState)
    {
        updateCameraBuffer(recordState);
        updateDirectionalLightsBuffer(recordState);
        updatePointLightsBuffer(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::Shutdown()
    {
        auto const deleteResult = EntitySystem::DestroyEntity(mRootEntity);
        MFA_ASSERT(deleteResult == true);
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

    RT::UniformBufferGroup const & Scene::GetCameraBuffers() const
    {
        return *mCameraBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight)
    {
        mPointLightComponents.emplace_back(pointLight);
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::RegisterDirectionalLight(std::weak_ptr<DirectionalLightComponent> const & directionalLight)
    {
        mDirectionalLightComponents.emplace_back(directionalLight);
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const & Scene::GetPointLightsBuffers() const
    {
        return *mPointLightsBuffers;
    }

    //-------------------------------------------------------------------------------------------------

    RT::UniformBufferGroup const & Scene::GetDirectionalLightBuffers() const
    {
        return *mDirectionalLightBuffers;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t Scene::GetPointLightCount() const
    {
        return mPointLightData.count;
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t Scene::GetDirectionalLightCount() const
    {
        return mDirectionalLightData.count;
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::RegisterPipeline(BasePipeline * pipeline)
    {
        mActivePipelines.emplace_back(pipeline);
    }

    //-------------------------------------------------------------------------------------------------

    BasePipeline * Scene::GetPipelineByName(std::string const & name) const
    {
        for (auto * pipeline : mActivePipelines)
        {
            if (pipeline->GetName() == name)
            {
                return pipeline;
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<BasePipeline *> Scene::GetPipelines() const
    {
        return mActivePipelines;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<PointLightComponent *> Scene::GetPointLights() const
    {
        std::vector<PointLightComponent *> pointLights {};
        for (auto & pointLightComponent : mPointLightComponents)
        {
            if (auto ptr = pointLightComponent.lock())
            {
                pointLights.emplace_back(ptr.get());
            }
        }
        return pointLights;
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::prepareCameraBuffer()
    {
        mCameraBuffer = RF::CreateUniformBuffer(
            sizeof(CameraComponent::CameraBufferData),
            RF::GetMaxFramesPerFlight()
        );

        CameraComponent::CameraBufferData const cameraBufferData{};

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                *mCameraBuffer->buffers[frameIndex],
                CBlobAliasOf(cameraBufferData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::preparePointLightsBuffer()
    {
        mPointLightsBuffers = RF::CreateUniformBuffer(
            sizeof(PointLightsBufferData),
            RF::GetMaxFramesPerFlight()
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                *mPointLightsBuffers->buffers[frameIndex],
                CBlobAliasOf(mPointLightData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::prepareDirectionalLightsBuffer()
    {
        mDirectionalLightBuffers = RF::CreateUniformBuffer(
            sizeof(DirectionalLightData),
            RF::GetMaxFramesPerFlight()
        );

        for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
        {
            RF::UpdateUniformBuffer(
                *mDirectionalLightBuffers->buffers[frameIndex],
                CBlobAliasOf(mDirectionalLightData)
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::updateCameraBuffer(RT::CommandRecordState const & recordState) const
    {
        if (auto const cameraPtr = mActiveCamera.lock())
        {
            if (cameraPtr->IsCameraDataDirty())
            {
                RF::UpdateUniformBuffer(
                    *mCameraBuffer->buffers[recordState.frameIndex],
                    CBlobAliasOf(cameraPtr->GetCameraData())
                );
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::updatePointLightsBuffer(RT::CommandRecordState const & recordState)
    {
        // Maybe we can search for another active camera 
        mPointLightData.count = 0;
        for (int i = static_cast<int>(mPointLightComponents.size()) - 1; i >= 0; --i)
        {
            if (mPointLightComponents[i].expired())
            {
                mPointLightComponents[i] = mPointLightComponents.back();
                mPointLightComponents.pop_back();
            }
        }
        for (auto & pointLightComponent : mPointLightComponents)
        {
            auto const ptr = pointLightComponent.lock();
            MFA_ASSERT(ptr != nullptr);
            if (ptr->IsVisible())
            {
                MFA_ASSERT(mPointLightData.count < MAX_POINT_LIGHT_COUNT);
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

        RF::UpdateUniformBuffer(
            recordState,
            *mPointLightsBuffers,
            CBlobAliasOf(mPointLightData)
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Scene::updateDirectionalLightsBuffer(RT::CommandRecordState const & recordState)
    {
        mDirectionalLightData.count = 0;
        for (int i = static_cast<int>(mDirectionalLightComponents.size()) - 1; i >= 0; --i)
        {
            if (mDirectionalLightComponents[i].expired())
            {
                mDirectionalLightComponents[i] = mDirectionalLightComponents.back();
                mDirectionalLightComponents.pop_back();
            }
        }

        for (auto & directionalLightComponent : mDirectionalLightComponents)
        {
            auto const ptr = directionalLightComponent.lock();
            MFA_ASSERT(ptr != nullptr);
            if (ptr->IsActive())
            {
                MFA_ASSERT(mDirectionalLightData.count < MAX_DIRECTIONAL_LIGHT_COUNT);
                auto & item = mDirectionalLightData.items[mDirectionalLightData.count];

                ptr->GetDirection(item.direction);
                ptr->GetColor(item.color);
                ptr->GetShadowViewProjectionMatrix(item.viewProjectionMatrix);
                
                ++mDirectionalLightData.count;
            }
        }

        RF::UpdateUniformBuffer(
            recordState,
            *mDirectionalLightBuffers,
            CBlobAliasOf(mDirectionalLightData)
        );
    }

    //-------------------------------------------------------------------------------------------------

}
