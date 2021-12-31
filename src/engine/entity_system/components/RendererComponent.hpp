#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class Variant;
    class BasePipeline;

    class RendererComponent : public Component {
    public:
    
        void notifyVariantDestroyed();

        void shutdown() override;

        void serialize(nlohmann::json & jsonObject) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        [[nodiscard]]
        Variant const * getVariant() const;

        [[nodiscard]]
        Variant * getVariant();

    protected:

        explicit RendererComponent();

        explicit RendererComponent(BasePipeline & pipeline, Variant * variant);

        BasePipeline * mPipeline = nullptr;
        Variant * mVariant = nullptr;

    };
    
}
