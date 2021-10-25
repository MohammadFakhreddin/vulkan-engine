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

void MFA::BoundingVolumeComponent::Update(float const deltaTimeInSec, RT::CommandRecordState const & recordState)
{
    Component::Update(deltaTimeInSec, recordState);

    auto const activeScene = SceneManager::GetActiveScene().lock();
    if (activeScene == nullptr)
    {
        return;
    }
    auto const activeCamera = activeScene->GetActiveCamera().lock();
    if (activeCamera == nullptr)
    {
        return;
    }
    mIsInFrustum = IsInsideCameraFrustum(activeCamera.get());
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
