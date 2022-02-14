#include "RendererComponent.hpp"

#include <utility>

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::shutdown()
{
    Component::shutdown();
    if (auto const variant = mVariant.lock())
    {
        mPipeline->removeVariant(*variant);
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::serialize(nlohmann::json & jsonObject) const
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

void MFA::RendererComponent::deserialize(nlohmann::json const & jsonObject)
{
    // TODO: Try to move deserialize to somewhere else
    std::string const pipelineName = jsonObject["pipeline"];
    mPipeline = SceneManager::GetPipeline(pipelineName);
    MFA_ASSERT(mPipeline != nullptr);

    std::string const address = jsonObject["address"];

    std::string relativePath{};
    Path::RelativeToAssetFolder(address, relativePath);

    if (mPipeline->essenceExists(relativePath) == false)
    {
        bool addResult = false;
        if (pipelineName == PBRWithShadowPipelineV2::Name)
        {
            auto const cpuModel = RC::AcquireCpuModel(relativePath);
            MFA_ASSERT(cpuModel != nullptr);
            auto const gpuModel = RC::AcquireGpuModel(relativePath);
            MFA_ASSERT(gpuModel != nullptr);

            auto const pbrMeshData = static_cast<AS::PBR::Mesh *>(cpuModel->mesh.get())->getMeshData();
            addResult = mPipeline->addEssence(std::make_shared<PBR_Essence>(gpuModel, pbrMeshData));
        }
        else if (pipelineName == ParticlePipeline::Name)
        {
            MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        }
        else if (pipelineName == DebugRendererPipeline::Name)
        {
            auto const cpuModel = RC::AcquireCpuModel(relativePath);       // TODO We need acquire mesh in a separate function
            MFA_ASSERT(cpuModel != nullptr);
            auto const gpuModel = RC::AcquireGpuModel(relativePath);
            MFA_ASSERT(gpuModel != nullptr);
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

MFA::VariantBase const * MFA::RendererComponent::getVariant() const
{
    return mVariant.lock().get();
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase * MFA::RendererComponent::getVariant()
{
    return mVariant.lock().get();
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, std::weak_ptr<VariantBase> variant)
    : mPipeline(&pipeline)
    , mVariant(std::move(variant))
{}

//-------------------------------------------------------------------------------------------------
