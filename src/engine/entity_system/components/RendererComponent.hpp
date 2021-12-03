#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

    class DrawableVariant;
    class BasePipeline;

    class RendererComponent : public Component {
    public:

        void NotifyVariantDestroyed();

        void Shutdown() override;

    protected:

        explicit RendererComponent(BasePipeline & pipeline, DrawableVariant * drawableVariant);

        BasePipeline * mPipeline = nullptr;
        DrawableVariant * mVariant = nullptr;

    };
    
}
