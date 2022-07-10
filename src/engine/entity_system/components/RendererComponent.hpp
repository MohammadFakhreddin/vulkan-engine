#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class VariantBase;
    class BasePipeline;

    class RendererComponent : public Component {
    public:

        MFA_ABSTRACT_COMPONENT_PROPS(
            RendererComponent,
            FamilyType::Invalid,
            EventTypes::ShutdownEvent,
            Component
        )

        void Shutdown() override;

        [[nodiscard]]
        std::weak_ptr<VariantBase> getVariant();

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    protected:
        
        std::string mNameId {};
        BasePipeline * mPipeline = nullptr;
        std::weak_ptr<VariantBase> mVariant {};

    };

    using Renderer = RendererComponent;
    
}
