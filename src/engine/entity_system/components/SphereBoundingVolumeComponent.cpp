#include "SphereBoundingVolumeComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

MFA::SphereBoundingVolumeComponent::SphereBoundingVolumeComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::SphereBoundingVolumeComponent::~SphereBoundingVolumeComponent() = default;

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
    MFA_ASSERT(mTransformComponent.expired() == false);

}

//-------------------------------------------------------------------------------------------------

MFA::BoundingVolumeComponent::DEBUG_CenterAndRadius MFA::SphereBoundingVolumeComponent::DEBUG_GetCenterAndRadius()
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<SphereBoundingVolumeComponent>(mRadius);
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Serialize(nlohmann::json & jsonObject) const
{
    jsonObject["radius"] = mRadius;
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Deserialize(nlohmann::json const & jsonObject)
{
    mRadius = jsonObject["radius"];
}

//-------------------------------------------------------------------------------------------------

bool MFA::SphereBoundingVolumeComponent::IsInsideCameraFrustum(CameraComponent const * camera)
{
    MFA_ASSERT(camera != nullptr);
    // TODO
    return true;
}

//-------------------------------------------------------------------------------------------------

