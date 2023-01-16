#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class VariantBase;
    class BaseMaterial;

    class RendererComponent : public Component {
    public:

        MFA_ABSTRACT_COMPONENT_PROPS(
            RendererComponent,
            EventTypes::ShutdownEvent,
            Component
        )

        void Shutdown() override;

        [[nodiscard]]
        std::weak_ptr<VariantBase> getVariant();

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        std::string GetNameId() const;

    protected:
        
        std::string mNameId {};
        BaseMaterial * mPipeline = nullptr;
        std::weak_ptr<VariantBase> mVariant {};

    };

    using Renderer = RendererComponent;
    
}
