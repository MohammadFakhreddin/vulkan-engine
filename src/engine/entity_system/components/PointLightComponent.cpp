#include "PointLightComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"

#include "libs/nlohmann/json.hpp"

#include <utility>

static constexpr float ProjectionNearDistance = 0.001f;

//-------------------------------------------------------------------------------------------------

MFA::PointLightComponent::PointLightComponent(
    float const radius,
    float const maxDistance,
    std::weak_ptr<MeshRendererComponent> attachedMesh
)
    : mRadius(radius)
    , mMaxDistance(maxDistance)
    , mProjectionNearDistance(ProjectionNearDistance)
    , mProjectionFarDistance(maxDistance)
    , mMaxSquareDistance(maxDistance * maxDistance)
    , mAttachedMesh(std::move(attachedMesh))
{
}

//-------------------------------------------------------------------------------------------------

MFA::PointLightComponent::~PointLightComponent() = default;

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Init()
{
    // Finding component references
    mColorComponent = GetEntity()->GetComponent<ColorComponent>();
    MFA_ASSERT(mColorComponent.expired() == false);

    mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent.expired() == false);
    mTransformChangeListenerId = mTransformComponent.lock()->RegisterChangeListener([this]()->void {
        computeViewProjectionMatrices();    
    });

    // Registering point light to active scene
    auto const activeScene = SceneManager::GetActiveScene();
    MFA_ASSERT(activeScene != nullptr);
    SceneManager::RegisterPointLight(selfPtr());
    
    computeProjection();
    computeViewProjectionMatrices();
    computeAttenuation();
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Shutdown()
{
    Component::Shutdown();

    if (auto const ptr = mTransformComponent.lock())
    {
        ptr->UnRegisterChangeListener(mTransformChangeListenerId);
    }
}

//-------------------------------------------------------------------------------------------------

glm::vec3 MFA::PointLightComponent::GetLightColor() const
{
    if (auto const ptr = mColorComponent.lock())
    {
        return ptr->GetColor();
    }
    return {};
}

//-------------------------------------------------------------------------------------------------

glm::vec3 MFA::PointLightComponent::GetPosition() const
{
    if (auto const ptr = mTransformComponent.lock())
    {
        return ptr->GetWorldPosition();
    }
    return {};
}

//-------------------------------------------------------------------------------------------------

float MFA::PointLightComponent::GetRadius() const
{
    return mRadius;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::OnUI()
{
    if (UI::TreeNode("PointLight"))
    {
        Component::OnUI();

        if (UI::InputFloat("Radius", mRadius))
        {
            computeAttenuation();
        }

        if (UI::InputFloat("MaxDistance", mMaxDistance))
        {
            mMaxSquareDistance = mMaxDistance * mMaxDistance;
            mProjectionFarDistance = mMaxDistance;

            computeProjection();
        }

        UI::TreePop();
    }
}

//-------------------------------------------------------------------------------------------------

bool MFA::PointLightComponent::IsVisible() const
{
    if (IsActive() == false)
    {
        return false;
    }

    if (auto const meshPtr = mAttachedMesh.lock())
    {
        if (auto const variantPtr = meshPtr->getVariant().lock())
        {
            return variantPtr->IsVisible();
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------------

bool MFA::PointLightComponent::IsBoundingVolumeInRange(BoundingVolumeComponent const * bvComponent) const
{
    auto const transformComponent = mTransformComponent.lock();
    if (transformComponent == nullptr)
    {
        return false;
    }

    auto const & lightWP = transformComponent->GetWorldPosition();

    auto const & bvWP = bvComponent->GetWorldPosition();
    auto const bvRadius = bvComponent->GetRadius();

    auto const xDiff = bvWP.x - lightWP.x;
    auto const yDiff = bvWP.y - lightWP.y;
    auto const zDiff = bvWP.z - lightWP.z;

    float const squareDistance = ((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) - (bvRadius * bvRadius);

    return squareDistance <= mMaxSquareDistance;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::GetShadowViewProjectionMatrices(float outData[6][16]) const
{
    MFA_ASSERT(outData != nullptr);
    memcpy(
        outData,
        mShadowViewProjectionMatrices,
        sizeof(mShadowViewProjectionMatrices)
    );
}

//-------------------------------------------------------------------------------------------------

float MFA::PointLightComponent::GetMaxDistance() const
{
    return mMaxDistance;
}

//-------------------------------------------------------------------------------------------------

float MFA::PointLightComponent::GetMaxSquareDistance() const
{
    return mMaxSquareDistance;
}

//-------------------------------------------------------------------------------------------------

float MFA::PointLightComponent::GetLinearAttenuation() const
{
    return mLinearAttenuation;
}

//-------------------------------------------------------------------------------------------------

float MFA::PointLightComponent::GetQuadraticAttenuation() const
{
    return mQuadraticAttenuation;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Clone(Entity * entity) const
{
    // We do not currently support attached mesh. Maybe we should give up on it
    MFA_ASSERT(mAttachedMesh.expired() == true);
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<PointLightComponent>(
        mRadius,
        mMaxDistance,
        std::shared_ptr<MeshRendererComponent> {}
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Serialize(nlohmann::json & jsonObject) const
{
    jsonObject["Radius"] = mRadius;
    jsonObject["MaxDistance"] = mMaxDistance;
    jsonObject["ProjectionNearDistance"] = mProjectionNearDistance;
    jsonObject["ProjectionFarDistance"] = mProjectionFarDistance;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Deserialize(nlohmann::json const & jsonObject)
{
    MFA_ASSERT(jsonObject.contains("Radius"));
    mRadius = jsonObject.value("Radius", 0.0f);

    MFA_ASSERT(jsonObject.contains("MaxDistance"));
    mMaxDistance = jsonObject.value("MaxDistance", 0.0f);

    MFA_ASSERT(jsonObject.contains("ProjectionNearDistance"));
    mProjectionNearDistance = jsonObject.value("ProjectionNearDistance", 0.0f);
    
    MFA_ASSERT(jsonObject.contains("ProjectionFarDistance"));
    mProjectionFarDistance = jsonObject.value("ProjectionFarDistance", 0.0f);

    mMaxSquareDistance = mMaxDistance * mMaxDistance;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::computeProjection()
{
    static constexpr float FOV = 90.0f;          // We are cube so FOV must be exact 90 degree
    static_assert(RT::POINT_LIGHT_SHADOW_WIDTH == RT::POINT_LIGHT_SHADOW_HEIGHT);
    static constexpr float ratio = 1;
        
    Matrix::PreparePerspectiveProjectionMatrix(
        mShadowProjectionMatrix,
        ratio,
        FOV,
        mProjectionNearDistance,
        mProjectionFarDistance
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::computeAttenuation()
{
    mLinearAttenuation = 1.0f / mRadius;
    mQuadraticAttenuation = 1.0f / (mRadius * mRadius);
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::computeViewProjectionMatrices()
{
    auto const transformComponentPtr = mTransformComponent.lock();
    if (transformComponentPtr == nullptr)
    {
        return;
    }

    glm::vec3 const lightWP = transformComponentPtr->GetWorldPosition();

    Copy<16>(
        mShadowViewProjectionMatrices[0],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, -1.0, 0.0)
        )
    );

    Copy<16>(
        mShadowViewProjectionMatrices[1],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(-1.0, 0.0, 0.0),
            glm::vec3(0.0, -1.0, 0.0)
        )
    );

    Copy<16>(
        mShadowViewProjectionMatrices[2],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(0.0, 0.0, 1.0)
        )
    );

    Copy<16>(
        mShadowViewProjectionMatrices[3],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(0.0, -1.0, 0.0),
            glm::vec3(0.0, 0.0, -1.0)
        )
    );

    Copy<16>(
        mShadowViewProjectionMatrices[4],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(0.0, 0.0, 1.0),
            glm::vec3(0.0, -1.0, 0.0)
        )
    );

    Copy<16>(
        mShadowViewProjectionMatrices[5],
        mShadowProjectionMatrix * glm::lookAt(
            lightWP,
            lightWP + glm::vec3(0.0, 0.0, -1.0),
            glm::vec3(0.0, -1.0, 0.0)
        )
    );
}

//-------------------------------------------------------------------------------------------------
