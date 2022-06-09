#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"

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
