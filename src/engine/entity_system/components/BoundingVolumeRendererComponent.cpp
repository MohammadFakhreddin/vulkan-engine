#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

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
            pipeline.CreateVariant(*RC::AcquireForGpu("Cube", false)))
    {}

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::init()
    {
        RendererComponent::init();

        mBoundingVolumeComponent = GetEntity()->GetComponent<BoundingVolumeComponent>();

        auto * entity = GetEntity();

        static int RendererEntityId = 0;
        auto * childEntity = EntitySystem::CreateEntity(
            std::string("BoundingVolumeMeshRenderer" + std::to_string(RendererEntityId++)).c_str(),
            SceneManager::GetActiveScene()->GetRootEntity(),
            EntitySystem::CreateEntityParams {
                .serializable = false
            }
        );

        mRendererTransformComponent = childEntity->AddComponent<TransformComponent>();
        MFA_ASSERT(mRendererTransformComponent.expired() == false);

        EntitySystem::InitEntity(childEntity);

        mVariant->Init(entity, selfPtr(), mRendererTransformComponent, mBoundingVolumeComponent);
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Update(
        float const deltaTimeInSec,
        RT::CommandRecordState const & recordState
    )
    {
        RendererComponent::Update(deltaTimeInSec, recordState);

        auto const boundingVolumeComponent = mBoundingVolumeComponent.lock();
        if (boundingVolumeComponent == nullptr)
        {
            return;
        }

        auto const rendererTransformComponent = mRendererTransformComponent.lock();
        if (rendererTransformComponent == nullptr)
        {
            return;
        }

        auto const & bvWorldPosition = boundingVolumeComponent->GetWorldPosition();
        auto const & bvExtend = boundingVolumeComponent->GetExtend();

        rendererTransformComponent->UpdateTransform(
            bvWorldPosition,
            rendererTransformComponent->GetRotation(),
            bvExtend
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::shutdown()
    {
        RendererComponent::shutdown();


    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::onUI()
    {
        if (UI::TreeNode("BoundingVolumeRenderer"))
        {
            RendererComponent::onUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::clone(Entity * entity) const
    {
        auto * pipeline = dynamic_cast<DebugRendererPipeline *>(mPipeline);
        MFA_ASSERT(pipeline != nullptr);
        entity->AddComponent<BoundingVolumeRendererComponent>(*pipeline);
    }

    //-------------------------------------------------------------------------------------------------

}
