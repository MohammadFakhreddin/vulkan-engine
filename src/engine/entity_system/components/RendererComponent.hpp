#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class VariantBase;
    class BasePipeline;

    class RendererComponent : public Component {
    public:
    
        void Shutdown() override;

        [[nodiscard]]
        std::weak_ptr<VariantBase> getVariant();

    protected:

        explicit RendererComponent();

        std::string mNameId {};
        BasePipeline * mPipeline = nullptr;
        std::weak_ptr<VariantBase> mVariant {};

    };
    
}
