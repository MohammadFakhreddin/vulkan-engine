#pragma once

#include "engine/entity_system/Component.hpp"

#include <vec3.hpp>

namespace MFA {

class ColorComponent final : public Component
{

public:

    MFA_COMPONENT_PROPS(ColorComponent)
    MFA_COMPONENT_CLASS_TYPE_1(ClassType::ColorComponent)
    MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::EmptyEvent)

    explicit ColorComponent() = default;

    explicit ColorComponent(glm::vec3 const & color) : mColor(color) {}

    void SetColor(float color[3]);

    void SetColor(glm::vec3 const & color);

    void GetColor(float outColor[3]) const;

    [[nodiscard]]
    glm::vec3 const & GetColor() const;

    void OnUI() override;

private:

    glm::vec3 mColor {};
};

}
