#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    MeshRendererComponent::MeshRendererComponent(BasePipeline & pipeline, char const * essenceName)
        : mPipeline(&pipeline)
        , mVariant(mPipeline->CreateDrawableVariant(essenceName))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Init()
    {
        Component::Init();
        mVariant->Init(this, GetEntity()->GetComponent<TransformComponent>());
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::Shutdown()
    {
        if (mVariant != nullptr)
        {
            mPipeline->RemoveDrawableVariant(mVariant);
        }
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * MeshRendererComponent::GetVariant() const
    {
        return mVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshRendererComponent::NotifyVariantDestroyed()
    {
        mVariant = nullptr;
    }

    //-------------------------------------------------------------------------------------------------

}
