#include "CameraComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UISystem.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

void CameraComponent::OnUI()
{

    glm::vec3 eulerAngles = mEulerAngles;

    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::Rotate(rotationMatrix, eulerAngles);
  
    auto forwardDirection = -ForwardVector;
    forwardDirection = forwardDirection * rotationMatrix;
    forwardDirection = glm::normalize(forwardDirection);

    auto rightDirection = RightVector;
    rightDirection = rightDirection * rotationMatrix;
    rightDirection = glm::normalize(rightDirection);

    auto upDirection = -UpVector;
    upDirection = upDirection * rotationMatrix;
    upDirection = glm::normalize(upDirection);

    UI::InputFloat3("ForwardPlane", reinterpret_cast<float *>(&forwardDirection));
    UI::InputFloat3("RightPlane", reinterpret_cast<float *>(&rightDirection));
    UI::InputFloat3("UpPlane", reinterpret_cast<float *>(&upDirection));

    UI::InputFloat3("NearPlanePosition", reinterpret_cast<float *>(&mNearPlane.position));
    UI::InputFloat3("NearPlaneDirection", reinterpret_cast<float *>(&mNearPlane.direction));

    UI::InputFloat3("FarPlanePosition", reinterpret_cast<float *>(&mFarPlane.position));
    UI::InputFloat3("FarPlaneDirection", reinterpret_cast<float *>(&mFarPlane.direction));
    
    UI::InputFloat3("LeftPlanePosition", reinterpret_cast<float *>(&mLeftPlane.position));
    UI::InputFloat3("LeftPlaneDirection", reinterpret_cast<float *>(&mLeftPlane.direction));

    UI::InputFloat3("RightPlanePosition", reinterpret_cast<float *>(&mRightPlane.position));
    UI::InputFloat3("RightPlaneDirection", reinterpret_cast<float *>(&mRightPlane.direction));

    UI::InputFloat3("TopPlanePosition", reinterpret_cast<float *>(&mTopPlane.position));
    UI::InputFloat3("TopPlaneDirection", reinterpret_cast<float *>(&mTopPlane.direction));

    UI::InputFloat3("BottomPlanePosition", reinterpret_cast<float *>(&mBottomPlane.position));
    UI::InputFloat3("BottomPlaneDirection", reinterpret_cast<float *>(&mBottomPlane.direction));

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
    OnResize();
}

//-------------------------------------------------------------------------------------------------

void CameraComponent::Update(float const deltaTimeInSec)
{
    Component::Update(deltaTimeInSec);

    glm::vec3 eulerAngles = mEulerAngles;//{180.0f + mEulerAngles[0], 180.0f + mEulerAngles[1], 180.0f + mEulerAngles[2]};

    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::Rotate(rotationMatrix, eulerAngles);
    //auto rotationMatrix = GetTransform();

    auto forwardDirection = -ForwardVector;
    forwardDirection = forwardDirection * rotationMatrix;
    forwardDirection = glm::normalize(forwardDirection);

    auto rightDirection = RightVector;
    rightDirection = rightDirection * rotationMatrix;
    rightDirection = glm::normalize(rightDirection);

    auto upDirection = -UpVector;
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

    //MFA_ASSERT(mNearPlane.IsInFrontOfPlane((mFarPlane.position)));
    //MFA_ASSERT(mFarPlane.IsInFrontOfPlane((mFarPlane.position)));
    //MFA_ASSERT(mRightPlane.IsInFrontOfPlane((mFarPlane.position)));
    //MFA_ASSERT(mLeftPlane.IsInFrontOfPlane((mFarPlane.position)));
    //MFA_ASSERT(mTopPlane.IsInFrontOfPlane((mFarPlane.position)));
    //MFA_ASSERT(mBottomPlane.IsInFrontOfPlane((mFarPlane.position)));
 
    /*MFA_ASSERT(IsPointInsideFrustum(mNearPlane.position));
    MFA_ASSERT(IsPointInsideFrustum(mFarPlane.position));
    MFA_ASSERT(IsPointInsideFrustum(mRightPlane.position));
    MFA_ASSERT(IsPointInsideFrustum(mLeftPlane.position));
    MFA_ASSERT(IsPointInsideFrustum(mTopPlane.position));
    MFA_ASSERT(IsPointInsideFrustum(mBottomPlane.position));
    */
    //frustum.nearFace = { cam.Position + zNear * cam.Front, cam.Front };
    //frustum.farFace = { cam.Position + frontMultFar, -cam.Front };
    //frustum.rightFace = { cam.Position,
    //                        glm::cross(cam.Up,frontMultFar + cam.Right * halfHSide) };
    //frustum.leftFace = { cam.Position,
    //                        glm::cross(frontMultFar - cam.Right * halfHSide, cam.Up) };
    //frustum.topFace = { cam.Position,
    //                        glm::cross(cam.Right, frontMultFar - cam.Up * halfVSide) };
    //frustum.bottomFace = { cam.Position,
    //                        glm::cross(frontMultFar + cam.Up * halfVSide, cam.Right) };
}

//-------------------------------------------------------------------------------------------------

void CameraComponent::OnResize()
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

void CameraComponent::GetProjection(float outProjection[16]) {
    Matrix::CopyGlmToCells(mProjectionMatrix, outProjection);
}

//-------------------------------------------------------------------------------------------------

glm::mat4 const & CameraComponent::GetProjection() const {
    return mProjectionMatrix;
}

//-------------------------------------------------------------------------------------------------
