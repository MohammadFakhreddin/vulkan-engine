#pragma once

#include <vec3.hpp>

#include "engine/entity_system/Component.hpp"

namespace MFA {
    class CameraComponent;

    class BoundingVolumeComponent : public Component
    {
    public:

        MFA_COMPONENT_CLASS_TYPE_3(ClassType::BoundingVolumeComponent, ClassType::SphereBoundingVolumeComponent, ClassType::AxisAlignedBoundingBoxes)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::UpdateEvent | EventTypes::InitEvent)

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
