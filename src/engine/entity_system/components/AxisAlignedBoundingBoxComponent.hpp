#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA
{

    class DrawableEssence;
    class TransformComponent;

    class AxisAlignedBoundingBoxComponent final : public BoundingVolumeComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            AxisAlignedBoundingBoxComponent,
            FamilyType::BoundingVolumeComponent,
            EventTypes::UpdateEvent | EventTypes::InitEvent
        )
        
        explicit AxisAlignedBoundingBoxComponent(
            glm::vec3 const & center,
            glm::vec3 const & extend
        );
        
        explicit AxisAlignedBoundingBoxComponent();
        
        void Init() override;

        [[nodiscard]]
        DEBUG_CenterAndRadius DEBUG_GetCenterAndRadius() override;

        void OnUI() override;

        void Clone(Entity * entity) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Serialize(nlohmann::json & jsonObject) const override;

    protected:

        bool IsInsideCameraFrustum(CameraComponent const * camera) override;

    private:

        bool mIsAutoGenerated;

        glm::vec3 mCenter {};
        glm::vec3 mExtend {};

        std::weak_ptr<TransformComponent> mTransformComponent {};

    
    };

}
