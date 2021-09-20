#pragma once

#include <string>

#include "engine/BedrockMatrix.hpp"

namespace MFA {

class CameraBase {
public:

    static constexpr float ForwardVector[4] {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr float RightVector[4] {1.0f, 0.0f, 0.0f, 0.0f};

    virtual ~CameraBase() = default;

    struct Input {
        float mouseDeltaX = 0.0f;
        float mouseDeltaY = 0.0f;
        float forwardMove = 0.0f;
        float rightMove = 0.0f;
    };
    virtual void OnUpdate(float deltaTime) = 0;

    virtual void OnResize() = 0;

    virtual void GetProjection(float outProjectionMatrix[16]) = 0;

    [[nodiscard]]
    virtual glm::mat4 const & GetProjection() const = 0;

    virtual void GetTransform(float outTransformMatrix[16]) = 0;

    [[nodiscard]]
    virtual glm::mat4 const & GetTransform() const = 0;

    virtual void ForcePosition(float position[3]) = 0;

    virtual void ForceRotation(float eulerAngles[3]) = 0;

    virtual void OnUI() {}

    void SetName(char const * name);

    [[nodiscard]]
    std::string const & GetName() const noexcept;

    virtual void GetPosition(float position[3]) const = 0;

protected:

    std::string mName = "Camera";

};

}