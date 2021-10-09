#include "BoundingVolumeComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::Init()
{
    Component::Init();
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::Update(float const deltaTimeInSec)
{
    Component::Update(deltaTimeInSec);

    mIsInFrustum = IsInsideCameraFrustum(SceneManager::GetActiveScene()->GetActiveCamera());
}

//-------------------------------------------------------------------------------------------------

bool MFA::BoundingVolumeComponent::IsInFrustum() const
{
    return mIsInFrustum;
}

//-------------------------------------------------------------------------------------------------

bool MFA::BoundingVolumeComponent::IsInsideCameraFrustum(CameraComponent const * camera)
{
    MFA_ASSERT(camera != nullptr);
    return true;
}

//-------------------------------------------------------------------------------------------------
