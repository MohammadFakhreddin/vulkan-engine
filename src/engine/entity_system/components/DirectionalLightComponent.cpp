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

MFA::DirectionalLightComponent::DirectionalLightComponent() {}

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

    static constexpr float Z_NEAR = -1000.0f;
    static constexpr float Z_FAR = 1000.0f;

    int32_t const width = Scene::DIRECTIONAL_LIGHT_SHADOW_WIDTH / 50;
    int32_t const height = Scene::DIRECTIONAL_LIGHT_SHADOW_HEIGHT / 50;
    
    float const halfWidth = static_cast<float>(width) / 2.0f;
    float const halfHeight = static_cast<float>(height) / 2.0f;
    float const left = -halfWidth;
    float const right = +halfWidth;
    float const top = -halfHeight;
    float const bottom = +halfHeight;

    /*Matrix::PrepareOrthographicProjectionMatrix(
        mShadowProjectionMatrix,
        left,
        right,
        bottom,
        top,
        Z_NEAR,
        Z_FAR
    );*/
    mShadowProjectionMatrix = glm::ortho(left, right, bottom, top, Z_NEAR, Z_FAR);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::computeDirectionAndShadowViewProjection()
{
    auto const transformComponent = mTransformComponentRef.lock();
    if (transformComponent == nullptr)
    {
        return;
    }

    // Way 1
    auto const rotation = transformComponent->GetRotation();

    glm::mat4 directionRotationMatrix = glm::identity<glm::mat4>();
    Matrix::Rotate(directionRotationMatrix, rotation);

    mDirection = directionRotationMatrix * RT::ForwardVector;

    auto const frustumCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    auto const shadowViewMatrix = glm::lookAt(
        mDirection - frustumCenter,
        frustumCenter,
        glm::vec3(0,1,0)
    );
    mShadowViewProjectionMatrix = mShadowProjectionMatrix * shadowViewMatrix;

}

//-------------------------------------------------------------------------------------------------

