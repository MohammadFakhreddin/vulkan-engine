#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/SceneManager.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent()
        : BoundingVolumeRendererComponent(*SceneManager::GetPipeline<DebugRendererPipeline>())
    {}
 
    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline)
        : RendererComponent(
            pipeline,
            pipeline.createVariant(*RC::AcquireGpuModel("Cube", false)))
    {}

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::~BoundingVolumeRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::init()
    {
        RendererComponent::init();

        mBoundingVolumeComponent = GetEntity()->GetComponent<BoundingVolumeComponent>();

        auto * entity = GetEntity();

        static int RendererEntityId = 0;

        char nameBuffer[100] {};
        auto const nameLength = sprintf(nameBuffer, "BoundingVolumeMeshRenderer%d", RendererEntityId++);

        auto * childEntity = EntitySystem::CreateEntity(
            std::string(nameBuffer, nameLength),
            nullptr,
            EntitySystem::CreateEntityParams {
                .serializable = false
            }
        );

        mRendererTransform = childEntity->AddComponent<TransformComponent>();
        MFA_ASSERT(mRendererTransform.expired() == false);

        EntitySystem::InitEntity(childEntity);

        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        variant->Init(
            entity,
            selfPtr(),
            mRendererTransform,
            mBoundingVolumeComponent
        );
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Update(float const deltaTimeInSec)
    {
        RendererComponent::Update(deltaTimeInSec);

        auto const boundingVolumeComponent = mBoundingVolumeComponent.lock();
        if (boundingVolumeComponent == nullptr)
        {
            return;
        }

        auto const rendererTransformComponent = mRendererTransform.lock();
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

        if (auto const ptr = mRendererTransform.lock())
        {
            EntitySystem::DestroyEntity(ptr->GetEntity());
        }
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

    void BoundingVolumeRendererComponent::serialize(nlohmann::json & jsonObject) const {}

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::deserialize(nlohmann::json const & jsonObject) {}

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::clone(Entity * entity) const
    {
        auto * pipeline = dynamic_cast<DebugRendererPipeline *>(mPipeline);
        MFA_ASSERT(pipeline != nullptr);
        entity->AddComponent<BoundingVolumeRendererComponent>(*pipeline);
    }

    //-------------------------------------------------------------------------------------------------

}
