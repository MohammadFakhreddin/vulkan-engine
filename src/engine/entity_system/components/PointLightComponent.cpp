#include "PointLightComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/entity_system/Entity.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

MFA::PointLightComponent::PointLightComponent(
    float const radius,
    float const maxDistance,
    float const projectionNearDistance,
    float const projectionFarDistance,
    std::weak_ptr<MeshRendererComponent> attachedMesh
)
    : mRadius(radius)
    , mMaxDistance(maxDistance)
    , mProjectionNearDistance(projectionNearDistance)
    , mProjectionFarDistance(projectionFarDistance)
    , mMaxSquareDistance(maxDistance * maxDistance)
    , mAttachedMesh(std::move(attachedMesh))
{
}

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
    activeScene->RegisterPointLight(SelfPtr());
    
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
        return ptr->GetPosition();
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

        float radius = mRadius;
        UI::InputFloat("Radius", &radius);
        if (radius != mRadius)
        {
            mRadius = radius;
            computeAttenuation();
        }

        float maxDistance = mMaxDistance;
        UI::InputFloat("MaxDistance", &maxDistance);
        if (maxDistance != mMaxDistance)
        {
            mMaxDistance = maxDistance;
            mMaxSquareDistance = maxDistance * maxDistance;
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
    if (auto const ptr = mAttachedMesh.lock())
    {
        return ptr->GetVariant()->IsVisible();
    }
    return true;
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
        mProjectionNearDistance,
        mProjectionFarDistance,
        std::shared_ptr<MeshRendererComponent> {}
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::computeProjection()
{
    static constexpr float FOV = 90.0f;          // We are cube so FOV must be exact 90 degree
    static_assert(Scene::POINT_LIGHT_SHADOW_WIDTH == Scene::POINT_LIGHT_SHADOW_HEIGHT);
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

    auto const lightPositionVector = transformComponentPtr->GetPosition();

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, -1.0, 0.0)
        ),
        mShadowViewProjectionMatrices[0]
    );

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(-1.0, 0.0, 0.0),
            glm::vec3(0.0, -1.0, 0.0)
        ),
        mShadowViewProjectionMatrices[1]
    );

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(0.0, 0.0, 1.0)
        ),
        mShadowViewProjectionMatrices[2]
    );

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(0.0, -1.0, 0.0),
            glm::vec3(0.0, 0.0, -1.0)
        ),
        mShadowViewProjectionMatrices[3]
    );

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(0.0, 0.0, 1.0),
            glm::vec3(0.0, -1.0, 0.0)
        ),
        mShadowViewProjectionMatrices[4]
    );

    Matrix::CopyGlmToCells(
        mShadowProjectionMatrix * glm::lookAt(
            lightPositionVector,
            lightPositionVector + glm::vec3(0.0, 0.0, -1.0),
            glm::vec3(0.0, -1.0, 0.0)
        ),
        mShadowViewProjectionMatrices[5]
    );
}

//-------------------------------------------------------------------------------------------------
