#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

namespace MFA {

class CameraComponent : public Component {
public:

    MFA_COMPONENT_PROPS(
        CameraComponent,
        EventTypes::InitEvent | EventTypes::UpdateEvent | EventTypes::ShutdownEvent,
        Component
    )

    struct CameraBufferData
    {
//        float projectFarToNearDistance;
//        float viewportDimension[2];
//        float placeholder0;
//        float cameraPosition[3];
//        float placeholder1;
        float viewProjection[16];
    };
    static_assert(sizeof(CameraBufferData) == 64);
    
    struct Plane
    {
        glm::vec3 direction;
        glm::vec3 position;

        [[nodiscard]]
        bool IsInFrontOfPlane(glm::vec3 const & point, glm::vec3 const & extend) const;

    };
    
    ~CameraComponent() override = default;

    void Init() override;

    void Update(float deltaTimeInSec) override;

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

    void Clone(Entity * entity) const override;

    void Deserialize(nlohmann::json const & jsonObject) override;

    void Serialize(nlohmann::json & jsonObject) const override;
    
    [[nodiscard]]
    float GetProjectionFarToNearDistance() const;
    
    [[nodiscard]]
    glm::vec2 GetViewportDimension() const;

protected:

    explicit CameraComponent(float fieldOfView, float nearDistance, float farDistance);

    void updateViewTransformMatrix();

    void updateCameraBufferData();

private:

    void onResize();

protected:

    float mFieldOfView = 0.0f;
    float mNearDistance = 0.0f;
    float mFarDistance = 0.0f;
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

    // TODO We could have used transform component instead
    glm::vec3 mPosition {};
    glm::vec3 mEulerAngles {};  // TODO: Camera should use rotation instead

    uint32_t mCameraBufferUpdateCounter = 0;

    CameraBufferData mCameraBufferData {};

    int mResizeEventListenerId = 0;
    
    float mProjectionFarToNearDistance = 0.0f;
    
    glm::vec2 mViewportDimension {};

};

}
