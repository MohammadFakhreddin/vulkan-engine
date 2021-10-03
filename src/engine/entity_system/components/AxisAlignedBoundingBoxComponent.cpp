#include "AxisAlignedBoundingBoxComponent.hpp"

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::AxisAlignedBoundingBoxComponent::Init()
{
    BoundingVolumeComponent::Init();

    mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent != nullptr);
}

//-------------------------------------------------------------------------------------------------

bool MFA::AxisAlignedBoundingBoxComponent::IsInsideCameraFrustum(CameraComponent const * camera)
{
    // TODO
    return BoundingVolumeComponent::IsInsideCameraFrustum(camera);
}

//-------------------------------------------------------------------------------------------------
