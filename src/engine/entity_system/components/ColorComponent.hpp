#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>

namespace MFA {

class ColorComponent final : public Component
{

public:

    MFA_COMPONENT_PROPS(
        ColorComponent,
        FamilyType::Color,
        EventTypes::EmptyEvent
    )

    explicit ColorComponent() = default;

    explicit ColorComponent(glm::vec3 const & color) : mColor(color) {}

    void SetColor(float color[3]);

    void SetColor(glm::vec3 const & color);

    void GetColor(float outColor[3]) const;

    [[nodiscard]]
    glm::vec3 const & GetColor() const;

    void onUI() override;

    void clone(Entity * entity) const override;

    void serialize(nlohmann::json & jsonObject) const override;

    void deserialize(nlohmann::json const & jsonObject) override;

private:

    glm::vec3 mColor {};
};

}
