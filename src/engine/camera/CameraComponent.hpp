#pragma once

#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include "engine/BedrockRotation.hpp"

namespace MFA
{
    class TransformComponent;

    class CameraComponent : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            CameraComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            Component
        )

            struct CameraBufferData
        {
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
        
        void Shutdown() override;

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

        [[nodiscard]]
        TransformComponent * GetTransform() const;

        [[nodiscard]]
        glm::vec3 const & GetForward() const;

        [[nodiscard]]
        glm::vec3 const & GetRight() const;

        [[nodiscard]]
        glm::vec3 const & GetUp() const;
        
    protected:

        explicit CameraComponent(float fieldOfView, float nearDistance, float farDistance);

        void updateCameraBufferData();

        virtual void OnTransformChange();

    private:

        void UpdateDirectionVectors();

        void updateViewTransformMatrix();

        void UpdatePlanes();

        void onResize();

    protected:

        glm::vec3 mForward{};
        glm::vec3 mUp{};
        glm::vec3 mRight{};

        float mFieldOfView = 0.0f;
        float mNearDistance = 0.0f;
        float mFarDistance = 0.0f;
        float mAspectRatio = 0.0f;

        glm::mat4 mProjectionMatrix{};
        glm::mat4 mViewMatrix{};
        
        Plane mNearPlane{};
        Plane mFarPlane{};
        Plane mRightPlane{};
        Plane mLeftPlane{};
        Plane mTopPlane{};
        Plane mBottomPlane{};

        uint32_t mCameraBufferUpdateCounter = 0;

        CameraBufferData mCameraBufferData{};

        int mResizeEventListenerId = 0;

        float mProjectionFarToNearDistance = 0.0f;

        glm::vec2 mViewportDimension{};

        std::weak_ptr<TransformComponent> mTransformComponent;

        SignalId mTransformChangeListener{};

    };

}
