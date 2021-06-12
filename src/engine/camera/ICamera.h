#pragma once

#include "engine/BedrockMatrix.hpp"

namespace MFA {

class ICamera {
public:

    inline static const Vector4Float ForwardVector {0.0f, 0.0f, 1.0f, 0.0f};
    inline static const Vector4Float RightVector {1.0f, 0.0f, 0.0f, 0.0f};

    virtual ~ICamera() = default;

    struct Input {
        float mouseDeltaX = 0.0f;
        float mouseDeltaY = 0.0f;
        float forwardMove = 0.0f;
        float rightMove = 0.0f;
    };
    virtual void onNewFrame(float deltaTime) = 0;

    virtual void onResize() = 0;

    virtual void getProjection(float outProjectionMatrix[16]) = 0;

    [[nodiscard]]
    virtual Matrix4X4Float const & getProjection() const = 0;

    virtual void getTransform(float outTransformMatrix[16]) = 0;

    [[nodiscard]]
    virtual Matrix4X4Float const & getTransform() const = 0;

    virtual void forcePosition(float position[3]) = 0;

    virtual void forceRotation(float eulerAngles[3]) = 0;

    virtual void onUI() {};

};

}