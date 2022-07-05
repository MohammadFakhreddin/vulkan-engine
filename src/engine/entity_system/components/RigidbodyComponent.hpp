#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA
{
    class RigidbodyComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            RigidbodyComponent,
            FamilyType::Rigidbody,
            EventTypes::EmptyEvent
        )

        explicit RigidbodyComponent();

    private:
    };

    using Rigidbody = RigidbodyComponent;
}
