#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA {

class TransformComponent;

class SphereBoundingVolumeComponent final : public BoundingVolumeComponent
{
public:

    MFA_COMPONENT_PROPS(
        SphereBoundingVolumeComponent,
        FamilyType::BoundingVolumeComponent,
        EventTypes::UpdateEvent | EventTypes::InitEvent
    )

    explicit SphereBoundingVolumeComponent();
    ~SphereBoundingVolumeComponent() override;

    explicit SphereBoundingVolumeComponent(float radius);
    
    void Init() override;

    DEBUG_CenterAndRadius DEBUG_GetCenterAndRadius() override;

    void Clone(Entity * entity) const override;

    void Serialize(nlohmann::json & jsonObject) const override;

    void Deserialize(nlohmann::json const & jsonObject) override;

protected:

    bool IsInsideCameraFrustum(CameraComponent const * camera) override;

private:

    float mRadius = 0.0f;
    std::weak_ptr<TransformComponent> mTransformComponent {};

};

}
