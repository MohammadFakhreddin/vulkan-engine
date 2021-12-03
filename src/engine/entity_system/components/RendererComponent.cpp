#include "RendererComponent.hpp"

#include "engine/render_system/pipelines/BasePipeline.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::NotifyVariantDestroyed()
{
    mVariant = nullptr;
}

//-------------------------------------------------------------------------------------------------

void MFA::RendererComponent::Shutdown()
{
    Component::Shutdown();
    if (mVariant != nullptr)
    {
        mPipeline->RemoveDrawableVariant(*mVariant);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::RendererComponent::RendererComponent(BasePipeline & pipeline, DrawableVariant * drawableVariant)
    : mPipeline(&pipeline)
    , mVariant(drawableVariant)
{}

//-------------------------------------------------------------------------------------------------
