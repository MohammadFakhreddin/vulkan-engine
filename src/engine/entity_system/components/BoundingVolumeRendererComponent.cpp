#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::~BoundingVolumeRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline)
        : RendererComponent(
            pipeline,
            pipeline.CreateDrawableVariant(*RC::Acquire("Cube", false)))
    {}

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Init()
    {
        RendererComponent::Init();

        mBoundingVolumeComponent = GetEntity()->GetComponent<BoundingVolumeComponent>();

        auto * entity = GetEntity();

        auto * childEntity = EntitySystem::CreateEntity(
            "BoundingVolumeMeshRendererChild",
            entity,
            EntitySystem::CreateEntityParams {
                .serializable = false
            }
        );

        mChildTransformComponent = childEntity->AddComponent<TransformComponent>();
        MFA_ASSERT(mChildTransformComponent.expired() == false);

        EntitySystem::InitEntity(childEntity);

        mVariant->Init(entity, SelfPtr(), mChildTransformComponent, mBoundingVolumeComponent);
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Update(
        float const deltaTimeInSec,
        RT::CommandRecordState const & recordState
    )
    {
        RendererComponent::Update(deltaTimeInSec, recordState);

        auto const boundingVolumePtr = mBoundingVolumeComponent.lock();
        if (boundingVolumePtr == nullptr)
        {
            return;
        }
        auto const childTransform = mChildTransformComponent.lock();
        if (childTransform == nullptr)
        {
            return;
        }

        auto const centerAndRadius = boundingVolumePtr->DEBUG_GetCenterAndRadius();
        childTransform->UpdateTransform(
            centerAndRadius.center,
            childTransform->GetRotation(),
            glm::vec3(centerAndRadius.extend.x, centerAndRadius.extend.y, centerAndRadius.extend.z)
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::OnUI()
    {
        if (UI::TreeNode("BoundingVolumeRenderer"))
        {
            RendererComponent::OnUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Clone(Entity * entity) const
    {
        auto * pipeline = dynamic_cast<DebugRendererPipeline *>(mPipeline);
        MFA_ASSERT(pipeline != nullptr);
        entity->AddComponent<BoundingVolumeRendererComponent>(*pipeline);
    }

    //-------------------------------------------------------------------------------------------------

}
