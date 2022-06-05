#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/ui_system/UI_System.hpp"
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
            pipeline.createVariant("CubeStrip"))
    {}

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::~BoundingVolumeRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::LateInit()
    {
        RendererComponent::LateInit();

        auto const bvComponent = GetEntity()->GetComponent<BoundingVolumeComponent>().lock();
        MFA_ASSERT(bvComponent != nullptr);

        auto const bvTransform = bvComponent->GetVolumeTransform();
        MFA_ASSERT(bvTransform.expired() == false);

        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);
        variant->Init(
            GetEntity(),
            selfPtr(),
            bvTransform,
            bvComponent
        );
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
