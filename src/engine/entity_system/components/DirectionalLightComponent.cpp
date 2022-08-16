#include "DirectionalLightComponent.hpp"

#include "ColorComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightComponent::DirectionalLightComponent() = default;

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightComponent::~DirectionalLightComponent() = default;

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
    SceneManager::RegisterDirectionalLight(selfPtr());
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
    Copy<16>(outViewProjectionMatrix, mShadowViewProjectionMatrix);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::GetDirection(float outDirection[3]) const
{
    Copy<3>(outDirection, mDirection);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::GetColor(float outColor[3]) const
{
    if (auto const ptr = mColorComponentRef.lock())
    {
        Copy<3>(outColor, ptr->GetColor());
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::Clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<DirectionalLightComponent>();
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::Serialize(nlohmann::json & jsonObject) const
{}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::Deserialize(nlohmann::json const & jsonObject)
{}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightComponent::computeShadowProjection()
{
    int32_t const width = RT::DIRECTIONAL_LIGHT_PROJECTION_WIDTH;
    int32_t const height = RT::DIRECTIONAL_LIGHT_PROJECTION_HEIGHT;
    
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
    //auto const rotation = transformComponent->GetLocalRotationEulerAngles();

    //auto directionRotationMatrix = glm::identity<glm::mat4>();
    //Matrix::RotateWithEulerAngle(directionRotationMatrix, rotation);

    //mDirection = directionRotationMatrix * Copy<glm::vec4>(Math::ForwardVec3);

    auto const & rotationMatrix = transformComponent->GetWorldRotation().GetMatrix();

    mDirection = rotationMatrix * Copy<glm::vec4>(Math::ForwardVec3);

    auto const shadowViewMatrix = glm::lookAt(
        mDirection,
        glm::vec3(0.0f, 0.0f, 0.0f),    // TODO: Maybe center should be based on camera focus postion
        Math::UpVec3
    );
    mShadowViewProjectionMatrix = mShadowProjectionMatrix * shadowViewMatrix;

}

//-------------------------------------------------------------------------------------------------

