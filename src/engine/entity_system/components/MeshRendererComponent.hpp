#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"

namespace MFA
{

    class MeshRendererComponent final : public RendererComponent
    {
    public:

        MFA_COMPONENT_PROPS(MeshRendererComponent)
        MFA_COMPONENT_CLASS_TYPE(ClassType::MeshRendererComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::ShutdownEvent)
    
        explicit MeshRendererComponent(BasePipeline & pipeline, char const * essenceName);
        
        void Init() override;

        void Shutdown() override;

        [[nodiscard]]
        std::weak_ptr<DrawableVariant> const & GetVariant() const;
       
        void OnUI() override;

    private:

        BasePipeline * mPipeline = nullptr;
        std::weak_ptr<DrawableVariant> mVariant {};

    };

}
