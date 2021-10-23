#pragma once

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/render_system/RenderTypes.hpp"

namespace MFA {

class CameraComponent : public Component {
public:

    inline static const glm::vec4 ForwardVector {0.0f, 0.0f, 1.0f, 0.0f};
    inline static const glm::vec4 RightVector {1.0f, 0.0f, 0.0f, 0.0f};
    inline static const glm::vec4 UpVector {0.0f, 1.0f, 0.0f, 0.0f};

    struct CameraBufferData
    {
        float viewProjection[16];
        float cameraPosition[3];
        float projectFarToNearDistance;
        
    };
    static_assert(sizeof(CameraBufferData) == 80);

    struct Plane
    {
        glm::vec3 direction;
        glm::vec3 position;

        [[nodiscard]]
        bool IsInFrontOfPlane(glm::vec3 const & point, glm::vec3 const & extend) const;

    };
    
    ~CameraComponent() override = default;

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

    void Init() override;

    void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

    void OnResize();

    void Shutdown() override;

    void ForcePosition(float position[3]);

    void ForceRotation(float eulerAngles[3]);

    void GetPosition(float outPosition[3]) const;

    void GetRotation(float outEulerAngles[3]) const;

    void OnUI() override;

    [[nodiscard]]
    bool IsPointInsideFrustum(glm::vec3 const & point, glm::vec3 const & extend) const;

    [[nodiscard]]
    CameraBufferData const & GetCameraData() const;

    bool IsCameraDataDirty();

protected:

    explicit CameraComponent(float fieldOfView, float nearDistance, float farDistance);

    void updateViewTransformMatrix();

    void updateCameraBufferData();

    float const mFieldOfView;
    float const mNearDistance;
    float const mFarDistance;
    float mAspectRatio = 0.0f;

    glm::mat4 mProjectionMatrix {};
    glm::mat4 mViewMatrix {};
    bool mIsTransformDirty = true;

    Plane mNearPlane {};
    Plane mFarPlane {};
    Plane mRightPlane {};
    Plane mLeftPlane {};
    Plane mTopPlane {};
    Plane mBottomPlane {};

    glm::vec3 mPosition {};
    glm::vec3 mEulerAngles {};

    uint32_t mCameraBufferUpdateCounter = 0;

    CameraBufferData mCameraBufferData {};

};

}
