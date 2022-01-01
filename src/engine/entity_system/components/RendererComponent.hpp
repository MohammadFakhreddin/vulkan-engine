#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class VariantBase;
    class BasePipeline;

    class RendererComponent : public Component {
    public:
    
        void notifyVariantDestroyed();

        void shutdown() override;

        void serialize(nlohmann::json & jsonObject) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        [[nodiscard]]
        VariantBase const * getVariant() const;

        [[nodiscard]]
        VariantBase * getVariant();

    protected:

        explicit RendererComponent();

        explicit RendererComponent(BasePipeline & pipeline, VariantBase * variant);

        BasePipeline * mPipeline = nullptr;
        VariantBase * mVariant = nullptr;

    };
    
}
