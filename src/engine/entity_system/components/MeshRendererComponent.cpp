#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

#include "libs/nlohmann/json.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline * pipeline, std::string const & nameId)
        : RendererComponent()
    {
        mPipeline = pipeline;
        mNameId = nameId;
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mNameId.empty() == false);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Init()
    {
        RendererComponent::Init();

        mInitialized = true;

        RC::AcquireEssence(mNameId, mPipeline, [this](bool success)->void{
            if (MFA_VERIFY(success))
            {
                createVariant();
            }
        });
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Shutdown()
    {
        RendererComponent::Shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::OnUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::OnUI();
            UI::Text("Pipeline: %s", mPipeline->GetName());
            if (auto const variant = mVariant.lock())
            {
                variant->OnUI();
            }
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------
    // TODO: We can automatically serialize and deserialize using some macro
    void MeshRendererComponent::Serialize(nlohmann::json & jsonObject) const
    {
        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        auto const * essence = variant->getEssence();
        MFA_ASSERT(essence != nullptr);
        jsonObject["address"] = essence->getNameId();
        jsonObject["pipeline"] = mPipeline->GetName();
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_ASSERT(mInitialized == false);

        MFA_ASSERT(jsonObject.contains("pipeline"));
        std::string const pipelineName = jsonObject.value("pipeline", "");
        mPipeline = SceneManager::GetPipeline(pipelineName);
        MFA_ASSERT(mPipeline != nullptr);

        MFA_ASSERT(jsonObject.contains("address"));
        std::string const address = jsonObject.value("address", "");

        mNameId = Path::RelativeToAssetFolder(address);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        entity->AddComponent<MeshRendererComponent>(mPipeline, mNameId);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::createVariant()
    {
        mVariant = mPipeline->createVariant(mNameId);
        MFA_ASSERT(mVariant.expired() == false);

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

}
