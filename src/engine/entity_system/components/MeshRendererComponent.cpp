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
            UI::Text("Pipeline: %s", mPipeline->GetName());
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
        auto const * essence = variant->getEssence();
        MFA_ASSERT(essence != nullptr);
        jsonObject["address"] = essence->getNameId();
        jsonObject["pipeline"] = mPipeline->GetName();
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::deserialize(nlohmann::json const & jsonObject)
    {
        std::string const pipelineName = jsonObject["pipeline"];
        mPipeline = SceneManager::GetPipeline(pipelineName);
        MFA_ASSERT(mPipeline != nullptr);

        std::string const address = jsonObject["address"];

        std::string nameId = Path::RelativeToAssetFolder(address);

        if (mPipeline->hasEssence(nameId) == false)
        {
            bool addResult = false;

            auto const cpuModel = RC::AcquireCpuModel(nameId);
            MFA_ASSERT(cpuModel != nullptr);
            //auto const gpuModel = RC::AcquireGpuModel(relativePath);
            //MFA_ASSERT(gpuModel != nullptr);

            if (pipelineName == PBRWithShadowPipelineV2::Name)
            {
                auto const * pbrMesh = static_cast<AS::PBR::Mesh *>(cpuModel->mesh.get());
                addResult = mPipeline->addEssence(std::make_shared<PBR_Essence>(
                    nameId,
                    *pbrMesh,
                    RC::AcquireGpuTextures(cpuModel->textureIds)
                ));
            }
            else if (pipelineName == ParticlePipeline::Name)
            {
                MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
            }
            else if (pipelineName == DebugRendererPipeline::Name)
            {
                auto const * debugMesh = static_cast<AS::Debug::Mesh *>(cpuModel->mesh.get());
                addResult = mPipeline->addEssence(std::make_shared<DebugEssence>(
                    nameId,
                    *debugMesh
                ));
            }
            else
            {
                MFA_CRASH("Unhandled pipeline type detected");
            }
            MFA_ASSERT(addResult == true);
        }
        mVariant = mPipeline->createVariant(nameId);
        MFA_ASSERT(mVariant.expired() == false);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        auto const * essence = variant->getEssence();
        MFA_ASSERT(essence != nullptr);
        auto const & nameId = essence->getNameId();
        MFA_ASSERT(nameId.empty() == false);
        entity->AddComponent<MeshRendererComponent>(*mPipeline, nameId);
    }

    //-------------------------------------------------------------------------------------------------

}
