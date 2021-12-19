#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/BedrockPath.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::NotifyVariantDestroyed()
{
    mVariant = nullptr;
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Shutdown()
{
    Component::Shutdown();
    if (mVariant != nullptr)
    {
        mPipeline->RemoveDrawableVariant(*mVariant);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, DrawableVariant * drawableVariant)
    : mPipeline(&pipeline)
    , mVariant(drawableVariant)
{}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Serialize(nlohmann::json & jsonObject) const
{
    jsonObject["address"] = mVariant->GetEssence()->GetGpuModel()->address;
    jsonObject["pipeline"] = mPipeline->GetName();
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Deserialize(nlohmann::json const & jsonObject)
{
    mPipeline = SceneManager::GetActiveScene()->GetPipelineByName(jsonObject["pipeline"]);
    MFA_ASSERT(mPipeline != nullptr);

    std::string const address = jsonObject["address"];
    auto const gpuModel = RC::Acquire(address.c_str());
    MFA_ASSERT(gpuModel != nullptr);

    mPipeline->CreateEssenceIfNotExists(gpuModel);
    mVariant = mPipeline->CreateDrawableVariant(gpuModel->id);
    MFA_ASSERT(mVariant != nullptr);
}

//-------------------------------------------------------------------------------------------------
