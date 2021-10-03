#pragma once

#include <string>

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Component.hpp"

namespace MFA {

class CameraComponent : public Component {
public:

    static constexpr float ForwardVector[4] {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr float RightVector[4] {1.0f, 0.0f, 0.0f, 0.0f};

    virtual ~CameraComponent() override = default;

    static uint8_t GetClassType(ClassType outComponentTypes[3])
    {
        // TODO It should be reverse CameraComponent must return all its child 
        outComponentTypes[0] = ClassType::CameraComponent;
        outComponentTypes[1] = ClassType::ObserverCameraComponent;
        outComponentTypes[2] = ClassType::ThirdPersonCamera;
        return 3;
    }

    [[nodiscard]]
    EventType RequiredEvents() const override
    {
        return EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent;
    }

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

    explicit CameraComponent();

    std::string mName = "Camera";

};

}
