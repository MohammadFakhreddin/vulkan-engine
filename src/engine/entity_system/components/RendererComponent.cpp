#include "RendererComponent.hpp"

#include "engine/BedrockPath.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Shutdown()
{
    Component::Shutdown();
    if (auto const variant = mVariant.lock())
    {
        mPipeline->removeVariant(*variant);
    }
}

//-------------------------------------------------------------------------------------------------

std::weak_ptr<MFA::VariantBase> MFA::RendererComponent::getVariant()
{
    return mVariant;
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent() = default;

//-------------------------------------------------------------------------------------------------
// TODO: We can automatically serialize and deserialize using some macro
void MFA::RendererComponent::Serialize(nlohmann::json & jsonObject) const
{
    auto const variant = mVariant.lock();
    MFA_ASSERT(variant != nullptr);
    auto const * essence = variant->getEssence();
    MFA_ASSERT(essence != nullptr);
    jsonObject["address"] = essence->getNameId();
    jsonObject["pipeline"] = mPipeline->GetName();
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Deserialize(nlohmann::json const & jsonObject)
{
    MFA_ASSERT(jsonObject.contains("pipeline"));
    std::string const pipelineName = jsonObject.value("pipeline", "");
    mPipeline = SceneManager::GetPipeline(pipelineName);
    MFA_ASSERT(mPipeline != nullptr);

    MFA_ASSERT(jsonObject.contains("address"));
    std::string const address = jsonObject.value("address", "");

    mNameId = Path::RelativeToAssetFolder(address);
}

//-------------------------------------------------------------------------------------------------
