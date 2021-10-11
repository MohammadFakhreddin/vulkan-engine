#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline)
        : mPipeline(&pipeline)
        , mVariant(mPipeline->CreateDrawableVariant("Cube"))
    {
        MFA_ASSERT(mPipeline != nullptr);
        MFA_ASSERT(mVariant != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Init()
    {
        Component::Init();

        mBoundingVolumeComponent = GetEntity()->GetComponent<BoundingVolumeComponent>();

        auto * entity = EntitySystem::CreateEntity("Child", GetEntity());
        mChildTransformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(mChildTransformComponent != nullptr);
        EntitySystem::InitEntity(entity);

        mVariant->Init(this, mChildTransformComponent);
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Update(float const deltaTimeInSec)
    {
        Component::Update(deltaTimeInSec);

        auto const centerAndRadius = mBoundingVolumeComponent->DEBUG_GetCenterAndRadius();
        mChildTransformComponent->UpdateTransform(
            centerAndRadius.center,
            mChildTransformComponent->GetRotation(),
            glm::vec3(centerAndRadius.extend.x, centerAndRadius.extend.y, centerAndRadius.extend.z)
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Shutdown()
    {
        Component::Shutdown();
        if (mVariant != nullptr)
        {
            mPipeline->RemoveDrawableVariant(mVariant);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::NotifyVariantDestroyed()
    {
        mVariant = nullptr;
    }

    //-------------------------------------------------------------------------------------------------

}
