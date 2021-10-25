#pragma once

#include "ColorComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/entity_system/Entity.hpp"

#include <vec3.hpp>

namespace MFA {
    class MeshRendererComponent;

    class ColorComponent;
    class TransformComponent;

    class PointLightComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(PointLightComponent)
        MFA_COMPONENT_CLASS_TYPE_1(ClassType::PointLightComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent)

        explicit PointLightComponent(float radius = 1.0f);

        void Init() override;

        [[nodiscard]]
        glm::vec3 GetLightColor() const;

        [[nodiscard]]
        glm::vec3 GetPosition() const;

        [[nodiscard]]
        float GetRadius() const;

        void OnUI() override {} // Nothing to edit in this class

    private:

        std::weak_ptr<ColorComponent> mColorComponent {};
        std::weak_ptr<TransformComponent> mTransformComponent {};

        float const mRadius = 1.0f; // Radius is used for light attenuation

        std::weak_ptr<MeshRendererComponent> mMeshRendererComponent {};

    };

}
