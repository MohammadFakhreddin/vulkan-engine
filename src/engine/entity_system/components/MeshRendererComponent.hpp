#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{

    class MeshRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            MeshRendererComponent,
            FamilyType::MeshRendererComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        explicit MeshRendererComponent(BasePipeline & pipeline, RT::GpuModelId id);
        explicit MeshRendererComponent(BasePipeline & pipeline, RT::GpuModel const & gpuModel);

        void Init() override;

        [[nodiscard]]
        DrawableVariant const * GetVariant() const;

        [[nodiscard]]
        DrawableVariant * GetVariant();

        void OnUI() override;

        void Clone(Entity * entity) const override;
    
    };

}
