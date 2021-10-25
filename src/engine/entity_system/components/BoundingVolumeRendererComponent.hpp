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

        MFA_COMPONENT_PROPS(BoundingVolumeRendererComponent)
        MFA_COMPONENT_CLASS_TYPE_1(ClassType::BoundingVolumeRendererComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::UpdateEvent)

        explicit BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline);

        void Init() override;

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        void Shutdown() override;
        
        void OnUI() override;

    private:

        DebugRendererPipeline * mPipeline = nullptr;
        std::weak_ptr<DrawableVariant> mVariant {};

        std::weak_ptr<BoundingVolumeComponent> mBoundingVolumeComponent {};
        
        std::weak_ptr<TransformComponent> mChildTransformComponent {};

    };

}
