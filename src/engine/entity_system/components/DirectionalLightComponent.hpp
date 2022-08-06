#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/mat4x4.hpp>

namespace MFA
{
    class ColorComponent;
    class TransformComponent;

    class DirectionalLightComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            DirectionalLightComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            Component
        )

        explicit DirectionalLightComponent();
        ~DirectionalLightComponent() override;

        void Init() override;

        void Shutdown() override;
        
        void GetShadowViewProjectionMatrix(float outViewProjectionMatrix[16]) const;

        void GetDirection(float outDirection[3]) const;

        void GetColor(float outColor[3]) const;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;
   
    private:

        static constexpr float Z_NEAR = -1000.0f;
        static constexpr float Z_FAR = 1000.0f;

        void computeShadowProjection();
        void computeDirectionAndShadowViewProjection();

    private:

        glm::vec3 mDirection {};

        glm::mat4 mShadowProjectionMatrix {};
        glm::mat4 mShadowViewProjectionMatrix {};

        std::weak_ptr<TransformComponent> mTransformComponentRef {};
        std::weak_ptr<ColorComponent> mColorComponentRef {};

        int mTransformChangeListenerId = 0;
    };

    using DirectionalLight = DirectionalLightComponent;

}
