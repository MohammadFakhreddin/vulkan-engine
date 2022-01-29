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

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::shutdown()
{
    Component::shutdown();
    if (auto const variant = mVariant.lock())
    {
        mPipeline->RemoveVariant(*variant);
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
    mPipeline = SceneManager::GetActiveScene()->GetPipelineByName(jsonObject["pipeline"]);
    MFA_ASSERT(mPipeline != nullptr);

    std::string const address = jsonObject["address"];

    std::string relativePath {};
    if (Path::RelativeToAssetFolder(address, relativePath) == false)
    {
        relativePath = address;
    }

    if (mPipeline->EssenceExists(relativePath) == false)
    {
        auto const cpuModel = RC::AcquireCpuModel(relativePath);       // TODO We need acquire mesh and acquire texture as separate functions
        MFA_ASSERT(cpuModel != nullptr);
        auto const gpuModel = RC::AcquireGpuModel(relativePath);
        MFA_ASSERT(gpuModel != nullptr);
        mPipeline->CreateEssence(gpuModel, cpuModel->mesh);
    }
    mVariant = mPipeline->CreateVariant(relativePath);
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
