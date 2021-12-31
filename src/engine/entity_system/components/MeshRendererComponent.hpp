#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{
    class DrawableVariant;

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
        DrawableVariant const * getDrawableVariant() const;

        [[nodiscard]]
        DrawableVariant * getDrawableVariant();

        void onUI() override;

        void clone(Entity * entity) const override;

    private:

        DrawableVariant * mDrawableVariant = nullptr;
 
    };

}
