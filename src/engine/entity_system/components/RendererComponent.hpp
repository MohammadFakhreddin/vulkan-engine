#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class DrawableVariant;
    class BasePipeline;

    class RendererComponent : public Component {
    public:

        void NotifyVariantDestroyed();

        void Shutdown() override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    protected:

        explicit RendererComponent() = default;

        explicit RendererComponent(BasePipeline & pipeline, DrawableVariant * drawableVariant);

        BasePipeline * mPipeline = nullptr;
        DrawableVariant * mVariant = nullptr;

    };
    
}
