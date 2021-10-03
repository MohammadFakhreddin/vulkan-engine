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

    mIsInFrustum = IsInsideCameraFrustum(SceneManager::GetActiveScene()->GetActiveCamera()); // TODO We need global reference to active camera
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
