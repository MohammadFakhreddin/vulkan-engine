#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/ui_system/UISystem.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, RT::GpuModelId const id)
        : RendererComponent(pipeline, pipeline.CreateVariant(id))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, RT::GpuModel const & gpuModel)
        : MeshRendererComponent(pipeline, gpuModel.id)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::init()
    {
        Component::init();

        auto * entity = GetEntity();
        MFA_ASSERT(entity != nullptr);

        mPBR_Variant = dynamic_cast<PBR_Variant *>(mVariant);
        MFA_ASSERT(mPBR_Variant != nullptr);

        mPBR_Variant->Init(
            GetEntity(),
            selfPtr(),
            entity->GetComponent<TransformComponent>(),
            entity->GetComponent<BoundingVolumeComponent>()
        );
    }

    //-------------------------------------------------------------------------------------------------

    PBR_Variant const * MeshRendererComponent::getDrawableVariant() const
    {
        return mPBR_Variant;
    }

    //-------------------------------------------------------------------------------------------------

    PBR_Variant * MeshRendererComponent::getDrawableVariant()
    {
        return mPBR_Variant;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::onUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::onUI();
            mPBR_Variant->OnUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        MFA_ASSERT(mVariant != nullptr);
        auto const * essence = mVariant->GetEssence();
        MFA_ASSERT(essence != nullptr);
        auto const * gpuModel = essence->GetGpuModel();
        MFA_ASSERT(gpuModel != nullptr);
        entity->AddComponent<MeshRendererComponent>(*mPipeline, gpuModel->id);
    }

    //-------------------------------------------------------------------------------------------------

}
