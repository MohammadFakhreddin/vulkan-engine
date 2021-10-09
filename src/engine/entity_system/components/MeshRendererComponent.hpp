#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA
{

    class MeshRendererComponent final : public Component
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

    private:

        BasePipeline * mPipeline = nullptr;
        DrawableVariant * mVariant = nullptr;

    };

}
