#pragma once

#include "engine/entity_system/Component.hpp"

#include <mat4x4.hpp>

namespace MFA
{
    class ColorComponent;
    class TransformComponent;

    class DirectionalLightComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(DirectionalLightComponent)
        MFA_COMPONENT_CLASS_TYPE_1(ClassType::DirectionalLightComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::ShutdownEvent)

        explicit DirectionalLightComponent();

        void Init() override;

        void Shutdown() override;
        
        void GetShadowViewProjectionMatrix(float outViewProjectionMatrix[16]) const;

        void GetDirection(float outDirection[3]) const;

        void GetColor(float outColor[3]) const;
        
    private:

        void computeShadowProjection();
        void computeDirectionAndShadowViewProjection();
        
        glm::vec3 mDirection {};

        glm::mat4 mShadowProjectionMatrix {};
        glm::mat4 mShadowViewProjectionMatrix {};

        std::weak_ptr<TransformComponent> mTransformComponentRef {};
        std::weak_ptr<ColorComponent> mColorComponentRef {};

        int mTransformChangeListenerId = 0;
    };

}
