#include "PointLightComponent.hpp"

#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"

//-------------------------------------------------------------------------------------------------

MFA::PointLightComponent::PointLightComponent(float const radius)
    : mRadius(radius)
{}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightComponent::Init()
{
    // Finding component references
    mColorComponent = GetEntity()->GetComponent<ColorComponent>();
    MFA_ASSERT(mColorComponent.expired() == false);
    mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
    MFA_ASSERT(mTransformComponent.expired() == false);

    // Registering point light to active scene
    auto const & activeScene = SceneManager::GetActiveScene();
    MFA_ASSERT(activeScene.expired() == false);
    if (auto const ptr = activeScene.lock())
    {
        ptr->RegisterPointLight(SelfPtr());
    }
    
    // Trying to find mesh renderer component
    auto * entity = GetEntity();
    while (entity != nullptr)
    {
        mMeshRendererComponent = entity->GetComponent<MeshRendererComponent>();
        if (mMeshRendererComponent.expired())
        {
            break;
        }
        entity = entity->GetParent();
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


float MFA::PointLightComponent::GetRadius() const
{
    return mRadius;
}

//-------------------------------------------------------------------------------------------------

