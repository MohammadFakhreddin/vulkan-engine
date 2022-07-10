#include "SphereBoundingVolumeComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

MFA::SphereBoundingVolumeComponent::~SphereBoundingVolumeComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::SphereBoundingVolumeComponent::SphereBoundingVolumeComponent(
    float const radius,
    bool const occlusionEnabled
)
    : BoundingVolumeComponent(occlusionEnabled)
    , mRadius(radius)
{
    MFA_ASSERT(radius > 0.0f);       
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Init()
{
    BoundingVolumeComponent::Init();
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

void MFA::SphereBoundingVolumeComponent::Clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<SphereBoundingVolumeComponent>(mRadius, OcclusionEnabled());
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Serialize(nlohmann::json & jsonObject) const
{
    jsonObject["radius"] = mRadius;
}

//-------------------------------------------------------------------------------------------------

void MFA::SphereBoundingVolumeComponent::Deserialize(nlohmann::json const & jsonObject)
{
    MFA_ASSERT(jsonObject.contains("radius"));
    mRadius = jsonObject.value("radius", 0.0f);
}

//-------------------------------------------------------------------------------------------------

bool MFA::SphereBoundingVolumeComponent::IsInsideCameraFrustum(CameraComponent const * camera)
{
    MFA_ASSERT(camera != nullptr);
    // TODO
    return true;
}

//-------------------------------------------------------------------------------------------------

