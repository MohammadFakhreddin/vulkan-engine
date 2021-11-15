#include "DirectionalLightComponent.hpp"

#include "ColorComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightComponent::DirectionalLightComponent(
    float const zNear,
    float const zFar
)
    : mZNear(zNear)
    , mZFar(zFar)
{
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::Init()
{
    Component::Init();

    mTransformComponentRef = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponentRef.expired() == false);

    mColorComponentRef = GetEntity()->GetComponent<ColorComponent>();
    MFA_ASSERT(mColorComponentRef.expired() == false);

    computeShadowProjection();
    computeDirectionAndShadowViewProjection();
    
    mTransformChangeListenerId = mTransformComponentRef.lock()->RegisterChangeListener([this]()->void {
        computeDirectionAndShadowViewProjection();
    });

    // Registering directional light to active scene
    auto const activeScene = SceneManager::GetActiveScene().lock();
    MFA_ASSERT(activeScene != nullptr);
    activeScene->RegisterDirectionalLight(SelfPtr());
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::Shutdown()
{
    Component::Shutdown();
    if (auto const mTransformComponent = mTransformComponentRef.lock())
    {
        mTransformComponent->UnRegisterChangeListener(mTransformChangeListenerId);
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::GetShadowViewProjectionMatrix(float outViewProjectionMatrix[16]) const
{
    MFA_ASSERT(outViewProjectionMatrix != nullptr);
    Matrix::CopyGlmToCells(mShadowViewProjectionMatrix, outViewProjectionMatrix);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::GetDirection(float outDirection[3]) const
{
    Matrix::CopyGlmToCells(mDirection, outDirection);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::GetColor(float outColor[3]) const
{
    if (auto const ptr = mColorComponentRef.lock())
    {
        Matrix::CopyGlmToCells(ptr->GetColor(), outColor);
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::computeShadowProjection()
{
    int32_t width;
    int32_t height;
    RF::GetDrawableSize(width, height);
    MFA_ASSERT(width > 0);
    MFA_ASSERT(height > 0);

    float const halfWidth = static_cast<float>(width) / 2.0f;
    float const halfHeight = static_cast<float>(height) / 2.0f;
    float const left = -halfWidth;
    float const right = halfWidth;
    float const top = -halfHeight;
    float const bottom = halfHeight;

    mShadowProjectionMatrix = glm::ortho(left, right, bottom, top, mZNear, mZFar);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::computeDirectionAndShadowViewProjection()
{
    auto const transformComponent = mTransformComponentRef.lock();
    if (transformComponent == nullptr)
    {
        return;
    }

    auto const rotation = transformComponent->GetRotation();

    glm::mat4 shadowViewMatrix = glm::identity<glm::mat4>();
    Matrix::Rotate(shadowViewMatrix, rotation);

    mDirection = shadowViewMatrix * RT::ForwardVector;
    mShadowViewProjectionMatrix = mShadowProjectionMatrix * shadowViewMatrix;
}

//-------------------------------------------------------------------------------------------------

