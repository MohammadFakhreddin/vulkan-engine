#pragma once

#include "engine/entity_system/Component.hpp"

#include <vec3.hpp>
#include <vec4.hpp>

namespace MFA {
    class CameraComponent;

    class BoundingVolumeComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            BoundingVolumeComponent,
            FamilyType::BoundingVolumeComponent,
            EventTypes::UpdateEvent | EventTypes::InitEvent
        )

        explicit BoundingVolumeComponent();
        ~BoundingVolumeComponent() override;

        void Init() override;

        void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

        [[nodiscard]]
        bool IsInFrustum() const;

        [[nodiscard]]
        virtual glm::vec3 const & GetLocalPosition() const = 0;

        [[nodiscard]]
        virtual glm::vec3 const & GetExtend() const = 0;

        [[nodiscard]]
        virtual float GetRadius() const = 0;

        [[nodiscard]]
        virtual glm::vec4 const & GetWorldPosition() const = 0;

        
    protected:

        virtual bool IsInsideCameraFrustum(CameraComponent const * camera);

    private:

        bool mIsInFrustum = false;

    };

}
