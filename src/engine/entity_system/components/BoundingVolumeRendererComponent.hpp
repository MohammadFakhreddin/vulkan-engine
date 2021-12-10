#pragma once

#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{

    class TransformComponent;
    class DebugRendererPipeline;
    class DrawableVariant;
    class BoundingVolumeComponent;

    class BoundingVolumeRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            BoundingVolumeRendererComponent,
            FamilyType::BoundingVolumeRendererComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::UpdateEvent
        )
        
        explicit BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline);

        void Init() override;

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        void OnUI() override;

        void Clone(Entity * entity) const override;

    private:

        std::weak_ptr<BoundingVolumeComponent> mBoundingVolumeComponent {};
        std::weak_ptr<TransformComponent> mChildTransformComponent {};

    };

}
