#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::shutdown()
{
    Component::shutdown();
    if (auto const variant = mVariant.lock())
    {
        mPipeline->removeVariant(*variant);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase const * MFA::RendererComponent::getVariant() const
{
    return mVariant.lock().get();
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase * MFA::RendererComponent::getVariant()
{
    return mVariant.lock().get();
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, std::weak_ptr<VariantBase> variant)
    : mPipeline(&pipeline)
    , mVariant(std::move(variant))
{}

//-------------------------------------------------------------------------------------------------
