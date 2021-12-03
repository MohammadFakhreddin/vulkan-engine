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
        : RendererComponent(pipeline, pipeline.CreateDrawableVariant(id))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, RT::GpuModel const & gpuModel)
        : MeshRendererComponent(pipeline, gpuModel.id)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Init()
    {
        Component::Init();

        auto * entity = GetEntity();
        MFA_ASSERT(entity != nullptr);

        mVariant->Init(
            GetEntity(),
            SelfPtr(),
            entity->GetComponent<TransformComponent>(),
            entity->GetComponent<BoundingVolumeComponent>()
        );
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant const * MeshRendererComponent::GetVariant() const
    {
        return mVariant;
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * MeshRendererComponent::GetVariant()
    {
        return mVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::OnUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::OnUI();
            mVariant->OnUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

}
