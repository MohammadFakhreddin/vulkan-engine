#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{

    class TransformComponent;
    class DebugRendererPipeline;
    class PBR_Variant;
    class BoundingVolumeComponent;

    class BoundingVolumeRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            BoundingVolumeRendererComponent,
            FamilyType::BoundingVolumeRenderer,
            EventTypes::LateInitEvent | EventTypes::ShutdownEvent | EventTypes::InitEvent
        )

        explicit BoundingVolumeRendererComponent(std::string const & nameId = "CubeStrip");
        ~BoundingVolumeRendererComponent() override;

        void Init() override;

        void LateInit() override;
        
        void onUI() override;

        void serialize(nlohmann::json & jsonObject) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        void clone(Entity * entity) const override;

        void Shutdown() override;

    private:

        void createVariant();

        bool mEssenceLoaded = false;
        bool mLateInitIsCalled = false;

    };

}
