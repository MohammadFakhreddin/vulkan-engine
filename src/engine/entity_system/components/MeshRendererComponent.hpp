#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
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
            FamilyType::MeshRenderer,
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        // Should only be used by json serializer
        explicit MeshRendererComponent();
        explicit MeshRendererComponent(BasePipeline * pipeline, std::string const & nameId);

        void Init() override;

        void Shutdown() override;

        void OnUI() override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Clone(Entity * entity) const override;

    private:
        
        void createVariant();

        bool mInitialized = false;

    };

    using MeshRenderer = MeshRendererComponent;

}
