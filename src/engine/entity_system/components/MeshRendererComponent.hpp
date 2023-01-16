#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BaseMaterial.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{
    namespace AssetSystem
    {
        struct Model;
    }

    class PBR_Variant;

    class MeshRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            MeshRendererComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            RendererComponent
        )

        explicit MeshRendererComponent(BaseMaterial * pipeline, std::string const & nameId);

        void Init() override;

        void Shutdown() override;

        void OnUI() override;
        
        void Clone(Entity * entity) const override;

    private:
        
        void createVariant();

        bool mInitialized = false;

    };

    using MeshRenderer = MeshRendererComponent;

}
