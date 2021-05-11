#pragma once

#include "engine/BedrockMatrix.hpp"

namespace MFA {

class ICamera {
public:

    inline static const Vector4Float ForwardVector {1.0f, 0.0f, 0.0f, 0.0f};
    inline static const Vector4Float RightVector {1.0f, 0.0f, 0.0f, 0.0f};

    virtual ~ICamera() = default;

    struct Input {
        float mouseDeltaX = 0.0f;
        float mouseDeltaY = 0.0f;
        float forwardBackwardMove = 0.0f;
        float rightLeftMove = 0.0f;
    };
    virtual void applyInput(float deltaTime, Input const & input) = 0;

    virtual void onResize() = 0;

    virtual void getProjection(float outProjectionMatrix[16]) = 0;

    virtual void getTransform(float outTransformMatrix[16]) = 0;

    virtual void forcePosition(Vector3Float const & position) = 0;

    virtual void forceRotation(Vector3Float const & eulerAngles) = 0;

};

}