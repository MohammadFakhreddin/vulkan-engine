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

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, char const * essenceName)
        : mPipeline(&pipeline)
        , mVariant(mPipeline->CreateDrawableVariant(essenceName))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant.expired() == false);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Init()
    {
        Component::Init();

        auto * entity = GetEntity();
        MFA_ASSERT(entity != nullptr);

        mVariant.lock()->Init(
            GetEntity(),
            SelfPtr(),
            entity->GetComponent<TransformComponent>(),
            entity->GetComponent<BoundingVolumeComponent>()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Shutdown()
    {
        if (auto const ptr = mVariant.lock())
        {
            mPipeline->RemoveDrawableVariant(ptr.get());
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<DrawableVariant> const & MeshRendererComponent::GetVariant() const
    {
        return mVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::OnUI()
    {
        if (UI::TreeNode("MeshRenderer"))
        {
            RendererComponent::OnUI();
            if (auto const ptr = mVariant.lock())
            {
                ptr->OnUI();
            }
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

}
