#include "SphereBoundingVolumeComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"

//-------------------------------------------------------------------------------------------------

MFA::SphereBoundingVolumeComponent::SphereBoundingVolumeComponent(float const radius)
    : mRadius(radius)
{
    MFA_ASSERT(radius > 0.0f);       
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Init()
{
    BoundingVolumeComponent::Init();

    mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent != nullptr);

}

//-------------------------------------------------------------------------------------------------

MFA::BoundingVolumeComponent::DEBUG_CenterAndRadius MFA::SphereBoundingVolumeComponent::DEBUG_GetCenterAndRadius()
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

bool MFA::SphereBoundingVolumeComponent::IsInsideCameraFrustum(CameraComponent const * camera)
{
    MFA_ASSERT(camera != nullptr);
    // TODO
    return true;
}

//-------------------------------------------------------------------------------------------------

