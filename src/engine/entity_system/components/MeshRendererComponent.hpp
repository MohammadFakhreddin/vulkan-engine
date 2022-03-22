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

        // Should only be used by json serializer
        explicit MeshRendererComponent() = default;
        explicit MeshRendererComponent(BasePipeline & pipeline, std::string const & nameOrAddress);

        void init() override;

        void shutdown() override;

        void onUI() override;

        void serialize(nlohmann::json & jsonObject) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        void clone(Entity * entity) const override;

    };

}
