#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{

    class MeshRendererComponent final : public RendererComponent
    {
    public:

        [[nodiscard]]
        EventType RequiredEvents() const override
        {
            return EventTypes::InitEvent | EventTypes::ShutdownEvent;
        }

        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::MeshRendererComponent;
            return 1;
        }
        
        explicit MeshRendererComponent(BasePipeline & pipeline, char const * essenceName);
        
        void Init() override;

        void Shutdown() override;

        [[nodiscard]]
        DrawableVariant * GetVariant() const;
        
        void NotifyVariantDestroyed() override;

        void OnUI() override;

    private:

        BasePipeline * mPipeline = nullptr;
        DrawableVariant * mVariant = nullptr;

    };

}
