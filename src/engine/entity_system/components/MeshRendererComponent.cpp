#include "MeshRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"

using namespace MFA;

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
    mVariant->Init(this);
}

//-------------------------------------------------------------------------------------------------

void MeshRendererComponent::Shutdown()
{
    mPipeline->RemoveDrawableVariant(mVariant);
}

//-------------------------------------------------------------------------------------------------

DrawableVariant * MeshRendererComponent::GetVariant() const
{
    return mVariant;
}

/*
//-------------------------------------------------------------------------------------------------

void MeshRendererComponent::SetActiveAnimationIndex(int const nextAnimationIndex, DrawableVariant::AnimationParams const & params) const
{
    mVariant->SetActiveAnimationIndex(nextAnimationIndex, params);
}

//-------------------------------------------------------------------------------------------------

void MeshRendererComponent::SetActiveAnimation(char const * animationName, DrawableVariant::AnimationParams const & params) const
{
    mVariant->SetActiveAnimation(animationName, params);
}

//-------------------------------------------------------------------------------------------------

bool MeshRendererComponent::IsCurrentAnimationFinished() const
{
    return mVariant->IsCurrentAnimationFinished();
}
*/
//-------------------------------------------------------------------------------------------------
