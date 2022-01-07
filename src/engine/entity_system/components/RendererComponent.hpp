#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class VariantBase;
    class BasePipeline;

    class RendererComponent : public Component {
    public:
    
        void shutdown() override;

        void serialize(nlohmann::json & jsonObject) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        [[nodiscard]]
        VariantBase const * getVariant() const;

        [[nodiscard]]
        VariantBase * getVariant();

    protected:

        explicit RendererComponent();

        explicit RendererComponent(BasePipeline & pipeline, std::weak_ptr<VariantBase> variant);

        BasePipeline * mPipeline = nullptr;
        std::weak_ptr<VariantBase> mVariant {};

    };
    
}
