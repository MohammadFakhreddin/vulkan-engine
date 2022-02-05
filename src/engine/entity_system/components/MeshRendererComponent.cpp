#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/ui_system/UI_System.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, std::string const & nameOrAddress)
        : RendererComponent(pipeline, pipeline.CreateVariant(nameOrAddress))
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
        Component::init();

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
