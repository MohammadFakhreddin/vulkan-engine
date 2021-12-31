#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
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

        mDrawableVariant = dynamic_cast<DrawableVariant *>(mVariant);
        MFA_ASSERT(mDrawableVariant != nullptr);

        mDrawableVariant->Init(
            GetEntity(),
            selfPtr(),
            entity->GetComponent<TransformComponent>(),
            entity->GetComponent<BoundingVolumeComponent>()
        );
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant const * MeshRendererComponent::getDrawableVariant() const
    {
        return mDrawableVariant;
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * MeshRendererComponent::getDrawableVariant()
    {
        return mDrawableVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::onUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::onUI();
            mDrawableVariant->OnUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        entity->AddComponent<MeshRendererComponent>(*mPipeline, mVariant->GetEssence()->GetGpuModel()->id);
    }

    //-------------------------------------------------------------------------------------------------

}
