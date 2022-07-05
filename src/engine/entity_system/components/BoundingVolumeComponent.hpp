#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace MFA {
    class TransformComponent;
    class CameraComponent;

    class BoundingVolumeComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            BoundingVolumeComponent,
            FamilyType::BoundingVolume,
            EventTypes::UpdateEvent | EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        explicit BoundingVolumeComponent(bool occlusionCullingEnabled);
        ~BoundingVolumeComponent() override;

        void Init() override;

        void Update(float deltaTimeInSec) override;

        void Shutdown() override;

        void OnUI() override;

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

        std::weak_ptr<TransformComponent> GetVolumeTransform();

        [[nodiscard]]
        bool OcclusionEnabled() const;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    protected:

        virtual bool IsInsideCameraFrustum(CameraComponent const * camera);

    private:

        void updateFrustumVisibility();

        void updateVolumeTransform() const;

        bool mIsInFrustum = false;

        bool mOcclusionEnabled = false;

        std::weak_ptr<TransformComponent> mBvTransform {};

    };

    using BoundVolume = BoundingVolumeComponent;

}
