#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA {

class TransformComponent;
// TODO This component is not needed but a no bounding volume component can be useful
class SphereBoundingVolumeComponent final : public BoundingVolumeComponent
{
public:

    static uint8_t GetClassType(ClassType outComponentTypes[3])
    {
        outComponentTypes[0] = ClassType::SphereBoundingVolumeComponent;
        return 1;
    }

    explicit SphereBoundingVolumeComponent(float radius);

    void Init() override;

    DEBUG_CenterAndRadius DEBUG_GetCenterAndRadius() override;

protected:

    bool IsInsideCameraFrustum(CameraComponent const * camera) override;

private:

    float mRadius = 0.0f;
    TransformComponent * mTransformComponent {};

};

}
