#pragma once
#include "engine/BedrockCommon.hpp"
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

    explicit ColorComponent(std::initializer_list<float> const & color)
    {
        ::memcpy(mColor, color.begin(), sizeof(float) * 3);
    }

    void SetColor(float color[3])
    {
        Copy<3>(mColor, color);
    }

    void GetColor(float outColor[3]) const
    {
        Copy<3>(outColor, mColor);
    }

    [[nodiscard]]
    float const * GetColor() const
    {
        return mColor;
    }

private:

    float mColor[3] {};
};

}
