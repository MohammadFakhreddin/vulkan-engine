#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::notifyVariantDestroyed()
{
    mVariant = nullptr;
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::shutdown()
{
    Component::shutdown();
    if (mVariant != nullptr)
    {
        mPipeline->RemoveVariant(*mVariant);
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::serialize(nlohmann::json & jsonObject) const
{
    jsonObject["address"] = mVariant->GetEssence()->GetGpuModel()->address;
    jsonObject["pipeline"] = mPipeline->GetName();
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::deserialize(nlohmann::json const & jsonObject)
{
    mPipeline = SceneManager::GetActiveScene()->GetPipelineByName(jsonObject["pipeline"]);
    MFA_ASSERT(mPipeline != nullptr);

    std::string const address = jsonObject["address"];

    std::string relativePath {};
    if (Path::RelativeToAssetFolder(address.c_str(), relativePath) == false)
    {
        relativePath = address;
    }

    if (mPipeline->EssenceExists(relativePath) == false)
    {
        auto const cpuModel = RC::AcquireForCpu(relativePath.c_str());       // TODO We need acquire mesh and acquire texture as separate functions
        MFA_ASSERT(cpuModel != nullptr);
        auto const gpuModel = RC::AcquireForGpu(relativePath.c_str());
        MFA_ASSERT(gpuModel != nullptr);
        mPipeline->CreateEssence(gpuModel, cpuModel->mesh);
    }
    mVariant = mPipeline->CreateVariant(relativePath);
    MFA_ASSERT(mVariant != nullptr);
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase const * MFA::RendererComponent::getVariant() const
{
    return mVariant;
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase * MFA::RendererComponent::getVariant()
{
    return mVariant;
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, VariantBase * variant)
    : mPipeline(&pipeline)
    , mVariant(variant)
{}

//-------------------------------------------------------------------------------------------------
