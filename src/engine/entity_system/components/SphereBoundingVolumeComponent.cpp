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

void MFA::SphereBoundingVolumeComponent::init()
{
    BoundingVolumeComponent::init();

    mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent.expired() == false);

}

//-------------------------------------------------------------------------------------------------

glm::vec3 const & MFA::SphereBoundingVolumeComponent::GetExtend() const
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

glm::vec3 const & MFA::SphereBoundingVolumeComponent::GetLocalPosition() const
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

float MFA::SphereBoundingVolumeComponent::GetRadius() const
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

glm::vec4 const & MFA::SphereBoundingVolumeComponent::GetWorldPosition() const
{
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<SphereBoundingVolumeComponent>(mRadius);
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::serialize(nlohmann::json & jsonObject) const
{
    jsonObject["radius"] = mRadius;
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::deserialize(nlohmann::json const & jsonObject)
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

