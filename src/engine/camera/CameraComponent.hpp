#pragma once

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/render_system/RenderTypes.hpp"

namespace MFA {

class CameraComponent : public Component {
public:

    MFA_COMPONENT_CLASS_TYPE_3(ClassType::CameraComponent, ClassType::ObserverCameraComponent, ClassType::ThirdPersonCamera)
    MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent)

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

    void Init() override;

    void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) override;

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

private:

    void onResize();

protected:

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

    int mResizeEventListenerId = 0;

};

}
