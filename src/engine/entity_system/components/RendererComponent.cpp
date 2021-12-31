#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/FoundationAsset.hpp"

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

    auto const cpuModel = RC::AcquireForCpu(address.c_str());
    MFA_ASSERT(cpuModel != nullptr);
    auto const gpuModel = RC::AcquireForGpu(address.c_str());
    MFA_ASSERT(gpuModel != nullptr);

    mPipeline->CreateEssenceIfNotExists(gpuModel, cpuModel->mesh);
    mVariant = mPipeline->CreateVariant(gpuModel->id);
    MFA_ASSERT(mVariant != nullptr);
}

//-------------------------------------------------------------------------------------------------

MFA::Variant const * MFA::RendererComponent::getVariant() const
{
    return mVariant;
}

//-------------------------------------------------------------------------------------------------

MFA::Variant * MFA::RendererComponent::getVariant()
{
    return mVariant;
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, Variant * variant)
    : mPipeline(&pipeline)
    , mVariant(variant)
{}

//-------------------------------------------------------------------------------------------------
