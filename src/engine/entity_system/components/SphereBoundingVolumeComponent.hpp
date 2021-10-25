#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA {

class TransformComponent;
// TODO This component is not needed but a no bounding volume component can be useful
class SphereBoundingVolumeComponent final : public BoundingVolumeComponent
{
public:

    MFA_COMPONENT_PROPS(SphereBoundingVolumeComponent)
    MFA_COMPONENT_CLASS_TYPE_1(ClassType::SphereBoundingVolumeComponent)

    explicit SphereBoundingVolumeComponent(float radius);

    void Init() override;

    DEBUG_CenterAndRadius DEBUG_GetCenterAndRadius() override;

protected:

    bool IsInsideCameraFrustum(CameraComponent const * camera) override;

private:

    float mRadius = 0.0f;
    std::weak_ptr<TransformComponent> mTransformComponent {};

};

}
