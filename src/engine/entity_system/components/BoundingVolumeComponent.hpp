#pragma once

#include "engine/entity_system/Component.hpp"

namespace MFA {
    class CameraComponent;

    class BoundingVolumeComponent : public Component
    {
    public:
    
        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::BoundingVolumeComponent;
            outComponentTypes[1] = ClassType::SphereBoundingVolumeComponent;
            outComponentTypes[2] = ClassType::AxisAlignedBoundingBoxes;
            return 3;
        }

        [[nodiscard]]
        EventType RequiredEvents() const override
        {
            return EventTypes::UpdateEvent | EventTypes::InitEvent;
        }

        void Init() override;

        void Update(float deltaTimeInSec) override;

        [[nodiscard]]
        bool IsInFrustum() const;


    protected:

        virtual bool IsInsideCameraFrustum(CameraComponent const * camera);

    private:

        bool mIsInFrustum = false;

    };

}
