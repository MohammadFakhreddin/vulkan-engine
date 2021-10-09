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

        [[nodiscard]]
        EventType RequiredEvents() const override
        {
            return EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::UpdateEvent;
        }

        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::BoundingVolumeRendererComponent;
            return 1;
        }

        explicit BoundingVolumeRendererComponent(DebugRendererPipeline & pipeline);

        void Init() override;

        void Update(float deltaTimeInSec) override;

        void Shutdown() override;
        
        void NotifyVariantDestroyed() override;

    private:

        DebugRendererPipeline * mPipeline = nullptr;
        DrawableVariant * mVariant = nullptr;

        BoundingVolumeComponent * mBoundingVolumeComponent = nullptr;

        Entity * mEntity = nullptr;

        TransformComponent * mChildTransformComponent = nullptr;

    };

}
