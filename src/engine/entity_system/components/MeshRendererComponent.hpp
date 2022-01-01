#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{
    class PBR_Variant;

    class MeshRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            MeshRendererComponent,
            FamilyType::MeshRenderer,
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        explicit MeshRendererComponent() = default;
        explicit MeshRendererComponent(BasePipeline & pipeline, RT::GpuModelId id);
        explicit MeshRendererComponent(BasePipeline & pipeline, RT::GpuModel const & gpuModel);

        void init() override;

        [[nodiscard]]
        PBR_Variant const * getDrawableVariant() const;

        [[nodiscard]]
        PBR_Variant * getDrawableVariant();

        void onUI() override;

        void clone(Entity * entity) const override;

    private:

        PBR_Variant * mPBR_Variant = nullptr;
 
    };

}
