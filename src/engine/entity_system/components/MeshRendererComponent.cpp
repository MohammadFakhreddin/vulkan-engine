#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

#include "libs/nlohmann/json.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, std::string const & nameOrAddress)
        : RendererComponent(pipeline, pipeline.createVariant(nameOrAddress))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant.expired() == false);
    }

    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, RT::GpuModel const & gpuModel)
        : MeshRendererComponent(pipeline, gpuModel.nameOrAddress)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::init()
    {
        RendererComponent::init();

        auto * entity = GetEntity();
        MFA_ASSERT(entity != nullptr);

        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        variant->Init(
            GetEntity(),
            selfPtr(),
            entity->GetComponent<TransformComponent>(),
            entity->GetComponent<BoundingVolumeComponent>()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::shutdown()
    {
        RendererComponent::shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::onUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::onUI();
            if (auto const variant = mVariant.lock())
            {
                variant->OnUI();
            }
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::serialize(nlohmann::json & jsonObject) const
    {
        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        auto const * essence = variant->GetEssence();
        MFA_ASSERT(essence != nullptr);
        auto const * gpuModel = essence->getGpuModel();
        MFA_ASSERT(gpuModel != nullptr);
        jsonObject["address"] = gpuModel->nameOrAddress;
        jsonObject["pipeline"] = mPipeline->GetName();
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::deserialize(nlohmann::json const & jsonObject)
    {
        std::string const pipelineName = jsonObject["pipeline"];
        mPipeline = SceneManager::GetPipeline(pipelineName);
        MFA_ASSERT(mPipeline != nullptr);

        std::string const address = jsonObject["address"];

        std::string relativePath{};
        Path::RelativeToAssetFolder(address, relativePath);

        if (mPipeline->essenceExists(relativePath) == false)
        {
            bool addResult = false;

            auto const cpuModel = RC::AcquireCpuModel(relativePath);
            MFA_ASSERT(cpuModel != nullptr);
            auto const gpuModel = RC::AcquireGpuModel(relativePath);
            MFA_ASSERT(gpuModel != nullptr);

            if (pipelineName == PBRWithShadowPipelineV2::Name)
            {
                auto const pbrMeshData = static_cast<AS::PBR::Mesh *>(cpuModel->mesh.get())->getMeshData();
                addResult = mPipeline->addEssence(std::make_shared<PBR_Essence>(gpuModel, pbrMeshData));
            }
            else if (pipelineName == ParticlePipeline::Name)
            {
                MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
            }
            else if (pipelineName == DebugRendererPipeline::Name)
            {
                addResult = mPipeline->addEssence(std::make_shared<DebugEssence>(gpuModel, cpuModel->mesh->getIndexCount()));
            }
            else
            {
                MFA_CRASH("Unhandled pipeline type detected");
            }
            MFA_ASSERT(addResult == true);
        }
        mVariant = mPipeline->createVariant(relativePath);
        MFA_ASSERT(mVariant.expired() == false);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        auto const * essence = variant->GetEssence();
        MFA_ASSERT(essence != nullptr);
        auto const * gpuModel = essence->getGpuModel();
        MFA_ASSERT(gpuModel != nullptr);
        entity->AddComponent<MeshRendererComponent>(*mPipeline, *gpuModel);
    }

    //-------------------------------------------------------------------------------------------------

}
