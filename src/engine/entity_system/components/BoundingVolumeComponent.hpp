#pragma once

#include <vec3.hpp>

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

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        [[nodiscard]]
        bool IsInFrustum() const;

        struct DEBUG_CenterAndRadius
        {
            glm::vec3 center {};
            glm::vec3 extend {};
            glm::vec3 positionMin {};
            glm::vec3 positionMax {};
        };
        [[nodiscard]]
        virtual DEBUG_CenterAndRadius DEBUG_GetCenterAndRadius() = 0;
        
    protected:

        virtual bool IsInsideCameraFrustum(CameraComponent const * camera);

    private:

        bool mIsInFrustum = false;

    };

}
