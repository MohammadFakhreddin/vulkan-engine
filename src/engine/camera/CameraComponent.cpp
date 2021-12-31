#include "CameraComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/ui_system/UISystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::onUI()
    {
        Component::onUI();

        glm::vec3 & eulerAngles = mEulerAngles;
        UI::InputFloat3("EulerAngles", eulerAngles.data.data);

        UI::InputFloat3("Position", mPosition.data.data);

        auto rotationMatrix = glm::identity<glm::mat4>();
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
        UI::InputFloat3("BottomPlaneDirection", mBottomPlane.direction.data.data);

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

    void CameraComponent::clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");   
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");   
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::serialize(nlohmann::json & jsonObject) const
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

    void CameraComponent::init()
    {
        Component::init();
        mCameraBufferUpdateCounter = RF::GetMaxFramesPerFlight();
        mResizeEventListenerId = RF::AddResizeEventListener([this]()->void{ onResize(); });
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::Update(float const deltaTimeInSec, RT::CommandRecordState const & recordState)
    {
        Component::Update(deltaTimeInSec, recordState);

        updateViewTransformMatrix();

        glm::vec3 eulerAngles = mEulerAngles;

        auto rotationMatrix = glm::identity<glm::mat4>();
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

        const float halfVSide = mFarDistance * tanf(mFieldOfView * 0.5f);
        const float halfHSide = halfVSide * mAspectRatio;
        const glm::vec3 frontFarDistanceVector = mFarDistance * forwardDirection;

        glm::vec3 position = -mPosition;

        mNearPlane.position = position + static_cast<glm::vec3>(mNearDistance * forwardDirection);
        mNearPlane.direction = forwardDirection;

        mFarPlane.position = position + frontFarDistanceVector;
        mFarPlane.direction = -forwardDirection;

        mRightPlane.position = position;
        mRightPlane.direction = glm::normalize(glm::cross(
            static_cast<glm::vec3>(upDirection),
            frontFarDistanceVector + static_cast<glm::vec3>(rightDirection * halfHSide)
        ));

        mLeftPlane.position = position;
        mLeftPlane.direction = glm::normalize(glm::cross(
            frontFarDistanceVector - static_cast<glm::vec3>(rightDirection * halfHSide),
            static_cast<glm::vec3>(upDirection)
        ));

        mTopPlane.position = position;
        mTopPlane.direction = glm::normalize(glm::cross(
            static_cast<glm::vec3>(rightDirection),
            frontFarDistanceVector - static_cast<glm::vec3>(upDirection * halfVSide)
        ));

        mBottomPlane.position = position;
        mBottomPlane.direction = glm::normalize(glm::cross(
            frontFarDistanceVector + static_cast<glm::vec3>(upDirection * halfVSide),
            static_cast<glm::vec3>(rightDirection)
        ));
    }
    
    //-------------------------------------------------------------------------------------------------

    void CameraComponent::shutdown()
    {
        Component::shutdown();
        RF::RemoveResizeEventListener(mResizeEventListenerId);
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::ForcePosition(float position[3])
    {
        if (Matrix::IsEqual(mPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mPosition);
            mIsTransformDirty = true;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::ForceRotation(float eulerAngles[3])
    {
        if (Matrix::IsEqual(mPosition, eulerAngles) == false)
        {
            Matrix::CopyCellsToGlm(eulerAngles, mEulerAngles);
            mIsTransformDirty = true;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::GetPosition(float outPosition[3]) const
    {
        outPosition[0] = -1 * mPosition[0];
        outPosition[1] = -1 * mPosition[1];
        outPosition[2] = -1 * mPosition[2];
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::GetRotation(float outEulerAngles[3]) const
    {
        outEulerAngles[0] = mEulerAngles[0];
        outEulerAngles[1] = mEulerAngles[1];
        outEulerAngles[2] = mEulerAngles[2];
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::updateViewTransformMatrix()
    {

        if (mIsTransformDirty == false)
        {
            return;
        }
        
        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::Rotate(rotationMatrix, mEulerAngles);

        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mPosition);

        mViewMatrix = rotationMatrix * translateMatrix;

        updateCameraBufferData();
    
        mIsTransformDirty = false;

    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::updateCameraBufferData()
    {
        auto const viewProjectionMatrix = mProjectionMatrix * mViewMatrix;

        GetPosition(mCameraBufferData.cameraPosition);
        Matrix::CopyGlmToCells(viewProjectionMatrix, mCameraBufferData.viewProjection);
        mCameraBufferData.projectFarToNearDistance = abs(mFarDistance - mNearDistance);

        mCameraBufferUpdateCounter = RF::GetMaxFramesPerFlight();
    }

    //-------------------------------------------------------------------------------------------------

    void CameraComponent::onResize()
    {
        int32_t width;
        int32_t height;
        RF::GetDrawableSize(width, height);
        MFA_ASSERT(width > 0);
        MFA_ASSERT(height > 0);

        mAspectRatio = static_cast<float>(width) / static_cast<float>(height);

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

}
