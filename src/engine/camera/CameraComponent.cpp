#include "CameraComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::OnUI()
    {
        Component::OnUI();

        //if (UI::InputFloat<3>("EulerAngles", mEulerAngles))
        //{
        //    mIsTransformDirty = true;
        //}
        //if (UI::InputFloat<3>("Position", mPosition))
        //{
        //    mIsTransformDirty = true;
        //}

       /* auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::Rotate(rotationMatrix, eulerAngles);

        auto forwardDirection = -Math::ForwardVector;
        forwardDirection = forwardDirection * rotationMatrix;
        forwardDirection = glm::normalize(forwardDirection);

        auto rightDirection = Math::RightVector;
        rightDirection = rightDirection * rotationMatrix;
        rightDirection = glm::normalize(rightDirection);

        auto upDirection = -Math::UpVector;
        upDirection = upDirection * rotationMatrix;
        upDirection = glm::normalize(upDirection);

        UI::InputFloat3("ForwardPlane", forwardDirection.data.m128_f32);
        UI::InputFloat3("RightPlane", rightDirection.data.m128_f32);
        UI::InputFloat3("UpPlane", upDirection.data.m128_f32);

        UI::InputFloat3("NearPlanePosition", mNearPlane.position.data.data);
        UI::InputFloat3("NearPlaneDirection", mNearPlane.direction.data.data);

        UI::InputFloat3("FarPlanePosition", mFarPlane.position.data.data);
        UI::InputFloat3("FarPlaneDirection", mFarPlane.direction.data.data);

        UI::InputFloat3("LeftPlanePosition", mLeftPlane.position.data.data);
        UI::InputFloat3("LeftPlaneDirection", mLeftPlane.direction.data.data);

        UI::InputFloat3("RightPlanePosition", mRightPlane.position.data.data);
        UI::InputFloat3("RightPlaneDirection", mRightPlane.direction.data.data);

        UI::InputFloat3("TopPlanePosition", mTopPlane.position.data.data);
        UI::InputFloat3("TopPlaneDirection", mTopPlane.direction.data.data);

        UI::InputFloat3("BottomPlanePosition", mBottomPlane.position.data.data);
        UI::InputFloat3("BottomPlaneDirection", mBottomPlane.direction.data.data);*/

    }

    //-------------------------------------------------------------------------------------------------

    bool CameraComponent::IsPointInsideFrustum(glm::vec3 const & point, glm::vec3 const & extend) const
    {
        return
            mNearPlane.IsInFrontOfPlane(point, extend) &&
            mFarPlane.IsInFrontOfPlane(point, extend) &&
            mLeftPlane.IsInFrontOfPlane(point, extend) &&
            mRightPlane.IsInFrontOfPlane(point, extend) &&
            mTopPlane.IsInFrontOfPlane(point, extend) &&
            mBottomPlane.IsInFrontOfPlane(point, extend);
    }

    //-------------------------------------------------------------------------------------------------

    CameraComponent::CameraBufferData const & CameraComponent::GetCameraData() const
    {
        return mCameraBufferData;
    }

    //-------------------------------------------------------------------------------------------------

    bool CameraComponent::IsCameraDataDirty()
    {
        auto const isDirty = mCameraBufferUpdateCounter > 0;
        if (isDirty)
        {
            --mCameraBufferUpdateCounter;
        }
        return isDirty;
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");   
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");   
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Serialize(nlohmann::json & jsonObject) const
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");   
    }

    //-------------------------------------------------------------------------------------------------

    bool CameraComponent::Plane::IsInFrontOfPlane(glm::vec3 const & point, glm::vec3 const & extend) const
    {
        // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
        const float radius = extend.x * std::abs(direction.x) +
            extend.y * std::abs(direction.y) +
            extend.z * std::abs(direction.z);

        glm::vec3 const vecToPoint = point - position;
        return glm::dot(vecToPoint, direction) >= -radius;
    }

    //-------------------------------------------------------------------------------------------------

    CameraComponent::CameraComponent(
        float const fieldOfView,
        float const nearDistance,
        float const farDistance
    )
        : Component()
        , mFieldOfView(fieldOfView)
        , mNearDistance(nearDistance)
        , mFarDistance(farDistance)
    {
        onResize();
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Init()
    {
        Component::Init();

        mCameraBufferUpdateCounter = RF::GetMaxFramesPerFlight();
        mResizeEventListenerId = RF::AddResizeEventListener([this]()->void{ onResize(); });

        auto const transformComponent = GetEntity()->GetComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);
        mTransformComponent = transformComponent;
        OnTransformChange();
        mTransformChangeListener = transformComponent->RegisterChangeListener(
            [this](Transform::ChangeParams const & params)->void
            {
                OnTransformChange();
            }
        );
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::OnTransformChange()
    {
        UpdateDirectionVectors();
        updateViewTransformMatrix();
        UpdatePlanes();
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::UpdateDirectionVectors()
    {
        auto const transformComponent = mTransformComponent.lock();
        if (transformComponent == nullptr)
        {
            return;
        }

        auto const & rotationMat = transformComponent->GetWorldRotation().GetMatrix();
        // Question: Why camera bounds are reversed ?
        auto forwardDirection = -Math::ForwardVec4W0;
        forwardDirection = forwardDirection * rotationMat;

        auto rightDirection = Math::RightVec4W0;
        rightDirection = rightDirection * rotationMat;

        auto upDirection = -Math::UpVec4W0;
        upDirection = upDirection * rotationMat;

        mForward = forwardDirection;
        mRight = rightDirection;
        mUp = upDirection;
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Shutdown()
    {
        Component::Shutdown();
        RF::RemoveResizeEventListener(mResizeEventListenerId);
        if (auto const transformComponent = mTransformComponent.lock())
        {
            transformComponent->UnRegisterChangeListener(mTransformChangeListener);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::updateViewTransformMatrix()
    {
        auto const transformComponent = mTransformComponent.lock();
        if (transformComponent == nullptr)
        {
            return;
        }
        auto const & rotationMat = transformComponent->GetWorldRotation().GetMatrix();

        auto const & position = -transformComponent->GetWorldPosition();

        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, position);

        mViewMatrix = rotationMat * translateMatrix;

        updateCameraBufferData();
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::UpdatePlanes()
    {
        auto * transform = GetTransform();
        if (transform == nullptr)
        {
            return;
        }

        const float halfVSide = mFarDistance * tanf(mFieldOfView * 0.5f);
        const float halfHSide = halfVSide * mAspectRatio;
        const glm::vec3 frontFarDistanceVector = mFarDistance * mForward;

        glm::vec3 const position = transform->GetWorldPosition();

        mNearPlane.position = position + (mNearDistance * mForward);
        mNearPlane.direction = mForward;

        mFarPlane.position = position + frontFarDistanceVector;
        mFarPlane.direction = -mForward;

        mRightPlane.position = position;
        mRightPlane.direction = glm::normalize(glm::cross(
            mUp,
            frontFarDistanceVector + (mRight * halfHSide)
        ));

        mLeftPlane.position = position;
        mLeftPlane.direction = glm::normalize(glm::cross(
            frontFarDistanceVector - (mRight * halfHSide),
            mUp
        ));

        mTopPlane.position = position;
        mTopPlane.direction = glm::normalize(glm::cross(
            mRight,
            frontFarDistanceVector - (mUp * halfVSide)
        ));

        mBottomPlane.position = position;
        mBottomPlane.direction = glm::normalize(glm::cross(
            frontFarDistanceVector + (mUp * halfVSide),
            mRight
        ));
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::updateCameraBufferData()
    {
        auto const viewProjectionMatrix = mProjectionMatrix * mViewMatrix;

        Copy<16>(mCameraBufferData.viewProjection, viewProjectionMatrix);
        mProjectionFarToNearDistance = abs(mFarDistance - mNearDistance);

        mCameraBufferUpdateCounter = RF::GetMaxFramesPerFlight();
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::onResize()
    {
        int32_t width;
        int32_t height;
        // TODO I think we should use device capabilities instead
        RF::GetDrawableSize(width, height);
        MFA_ASSERT(width > 0);
        MFA_ASSERT(height > 0);

        mAspectRatio = static_cast<float>(width) / static_cast<float>(height);

        mViewportDimension.x = static_cast<float>(width);
        mViewportDimension.y = static_cast<float>(height);

        Matrix::PreparePerspectiveProjectionMatrix(
            mProjectionMatrix,
            mAspectRatio,
            mFieldOfView,
            mNearDistance,
            mFarDistance
        );

        updateCameraBufferData();

        // TODO We should handle orientation change on android/IOS (UI shader might need a change)
        // https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html
        //#ifdef __ANDROID__
        //    // For mobile
        //    glm::mat4 projectionMatrix;
        //    glm::mat4::ConvertMatrixToGlm(mProjectionMatrix, projectionMatrix);
        //
        //    static constexpr glm::vec3 rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        //    // TODO Should we cache it or it is not important ?
        //    auto const capabilities = RF::GetSurfaceCapabilities();
        //
        //    if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
        //        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(90.0f), rotationAxis);
        //    } else if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        //        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(270.0f), rotationAxis);
        //    } else if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
        //        projectionMatrix = glm::rotate(projectionMatrix, glm::radians(180.0f), rotationAxis);
        //    }
        //
        //    glm::mat4::ConvertGmToCells(projectionMatrix, mProjectionMatrix.cells);
        //#endif
    }

    //-------------------------------------------------------------------------------------------------

    float CameraComponent::GetProjectionFarToNearDistance() const {
        return mProjectionFarToNearDistance;
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    glm::vec2 CameraComponent::GetViewportDimension() const {
        return mViewportDimension;
    }

    //-------------------------------------------------------------------------------------------------

    TransformComponent * CameraComponent::GetTransform() const
    {
        auto const transformComponent = mTransformComponent.lock();
        if (transformComponent == nullptr)
        {
            MFA_ASSERT(false);
            return {};
        }
        return transformComponent.get();
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const& CameraComponent::GetForward() const
    {
        return mForward;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & CameraComponent::GetRight() const
    {
        return mRight;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const& CameraComponent::GetUp() const
    {
        return mUp;
    }

    //-------------------------------------------------------------------------------------------------

}
