#pragma once
#include <vec3.hpp>

#include "BoundingVolumeComponent.hpp"

namespace MFA
{

    class TransformComponent;

    class AxisAlignedBoundingBoxComponent final : public BoundingVolumeComponent
    {
    public:

        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::AxisAlignedBoundingBoxes;
            return 1;
        }
        
        explicit AxisAlignedBoundingBoxComponent(glm::vec3 const & extend)
            : mExtend(extend) {}

        void Init() override;

    protected:

        bool IsInsideCameraFrustum(CameraComponent const * camera) override;

    private:

        glm::vec3 mExtend{};

        TransformComponent * mTransformComponent = nullptr;

    };

}
