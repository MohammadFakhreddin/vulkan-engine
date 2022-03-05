#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace MFA {
    class CameraComponent;

    class BoundingVolumeComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            BoundingVolumeComponent,
            FamilyType::BoundingVolume,
            EventTypes::UpdateEvent | EventTypes::InitEvent
        )

        explicit BoundingVolumeComponent();
        ~BoundingVolumeComponent() override;

        void init() override;

        void Update(float deltaTimeInSec) override;

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
