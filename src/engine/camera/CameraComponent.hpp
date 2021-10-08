#pragma once

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Component.hpp"

namespace MFA {

class CameraComponent : public Component {
public:

    inline static const glm::vec4 ForwardVector {0.0f, 0.0f, 1.0f, 0.0f};
    inline static const glm::vec4 RightVector {1.0f, 0.0f, 0.0f, 0.0f};
    inline static const glm::vec4 UpVector {0.0f, 1.0f, 0.0f, 0.0f};

    struct Plane
    {
        glm::vec3 direction;
        glm::vec3 position;

        bool IsInFrontOfPlane(glm::vec3 const & point, glm::vec3 const & extend) const;

    };
    
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

    void Update(float deltaTimeInSec) override;

    void OnResize();

    virtual void GetProjection(float outProjectionMatrix[16]);

    [[nodiscard]]
    virtual glm::mat4 const & GetProjection() const;

    virtual void GetTransform(float outTransformMatrix[16]) = 0;

    [[nodiscard]]
    virtual glm::mat4 const & GetTransform() const = 0;

    virtual void ForcePosition(float position[3]) = 0;

    virtual void ForceRotation(float eulerAngles[3]) = 0;

    virtual void OnUI();

    virtual void GetPosition(float position[3]) const = 0;

    [[nodiscard]]
    bool IsPointInsideFrustum(glm::vec3 const & point, glm::vec3 const & extend) const;

protected:

    explicit CameraComponent(float fieldOfView, float nearDistance, float farDistance);

    float const mFieldOfView;
    float const mNearDistance;
    float const mFarDistance;
    float mAspectRatio = 0.0f;

    glm::mat4 mProjectionMatrix {};

    Plane mNearPlane {};
    Plane mFarPlane {};
    Plane mRightPlane {};
    Plane mLeftPlane {};
    Plane mTopPlane {};
    Plane mBottomPlane {};

    glm::vec3 mPosition {};
    glm::vec3 mEulerAngles {};
    

};

}
