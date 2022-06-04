#include "BoundingVolumeComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockString.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

MFA::BoundingVolumeComponent::BoundingVolumeComponent(bool const occlusionCullingEnabled)
    : mOcclusionEnabled(occlusionCullingEnabled)
{}

//-------------------------------------------------------------------------------------------------

MFA::BoundingVolumeComponent::~BoundingVolumeComponent() = default;

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::Init()
{
    Component::Init();
    
    static int NextTransformEntityId = 0;

    std::string entityName;
    MFA_STRING(
        entityName,
        "BoundingVolumeMeshRendererTransform (%d)",
        NextTransformEntityId++
    )

    auto * childEntity = EntitySystem::CreateEntity(
        entityName,
        nullptr,// It is set to null to prevent the scale to effect the bounding volume
        EntitySystem::CreateEntityParams {
            .serializable = false
        }
    );

    mBvTransform = childEntity->AddComponent<TransformComponent>();
    MFA_ASSERT(mBvTransform.expired() == false);

    EntitySystem::InitEntity(childEntity);
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::Update(float const deltaTimeInSec)
{
    Component::Update(deltaTimeInSec);

    // Checking if we are inside frustum or not
    auto const activeScene = SceneManager::GetActiveScene();
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

    // Updating BVTransform
    auto const bvTransform = mBvTransform.lock();
    if (bvTransform == nullptr)
    {
        return;
    }

    auto const & bvWorldPosition = GetWorldPosition();
    auto const & bvExtend = GetExtend();

    bvTransform->UpdateTransform(
        bvWorldPosition,
        bvTransform->GetRotation(),
        bvExtend
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::Shutdown()
{
    Component::Shutdown();

    if (auto const ptr = mBvTransform.lock())
    {
        EntitySystem::DestroyEntity(ptr->GetEntity());
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::onUI()
{
    Component::onUI();
    UI::Checkbox("Occlusion culling enabled", &mOcclusionEnabled);
    UI::Text("Is inside frustum: %s", mIsInFrustum ? "true" : "false");
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

std::weak_ptr<MFA::TransformComponent> MFA::BoundingVolumeComponent::GetVolumeTransform()
{
    return mBvTransform;
}

//-------------------------------------------------------------------------------------------------

bool MFA::BoundingVolumeComponent::OcclusionEnabled() const
{
    return mOcclusionEnabled;
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::serialize(nlohmann::json & jsonObject) const
{
    jsonObject["OcclusionEnabled"] = mOcclusionEnabled;
}

//-------------------------------------------------------------------------------------------------

void MFA::BoundingVolumeComponent::deserialize(nlohmann::json const & jsonObject)
{
    mOcclusionEnabled = jsonObject.value<bool>("OcclusionEnabled", false);
}

//-------------------------------------------------------------------------------------------------
