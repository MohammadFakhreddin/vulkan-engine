#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {

class ColorComponent final : public Component
{

public:

    static uint8_t GetClassType(ClassType outComponentTypes[3])
    {
        outComponentTypes[0] = ClassType::ColorComponent;
        return 1;
    }

    [[nodiscard]]
    EventType RequiredEvents() const override
    {
        return EventTypes::EmptyEvent;
    }

    explicit ColorComponent() = default;

    explicit ColorComponent(glm::vec3 const & color) : mColor(color) {}

    void SetColor(float color[3])
    {
        Matrix::CopyCellsToGlm(color, mColor);
    }

    void SetColor(glm::vec3 const & color)
    {
        mColor = color;
    }

    void GetColor(float outColor[3]) const
    {
        Matrix::CopyGlmToCells(mColor, outColor);
    }

    [[nodiscard]]
    glm::vec3 const & GetColor() const
    {
        return mColor;
    }

private:

    glm::vec3 mColor {};
};

}
