#include "ThirdPersonCamera.hpp"

#include "engine/InputManager.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMath.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

ThirdPersonCamera::ThirdPersonCamera(
    float const fieldOfView,
    float const farPlane,
    float const nearPlane,
    float const rotationSpeed
)
    : mFieldOfView(fieldOfView)
    , mFarPlane(farPlane)
    , mNearPlane(nearPlane)
    , mRotationSpeed(rotationSpeed)
{}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::Init(
    DrawableVariant * variant, 
    float const distance, 
    float eulerAngles[3]
) {
    mVariant = variant;
    MFA_ASSERT(mVariant != nullptr);

    MFA_ASSERT(distance >= 0);
    mDistance = distance;

    Copy<3>(mEulerAngles, eulerAngles);

    OnResize();
    updateTransform();

    auto const surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const screenWidth = surfaceCapabilities.currentExtent.width;
    auto const screenHeight = surfaceCapabilities.currentExtent.height;
    RF::WarpMouseInWindow(
        static_cast<int>(static_cast<float>(screenWidth) / 2.0f), 
        static_cast<int>(static_cast<float>(screenHeight) / 2.0f)
    );
    IM::WarpMouseAtEdges(true);
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::OnUpdate(float const deltaTimeInSec) {
    {// Rotation
        auto const mouseDeltaX = IM::GetMouseDeltaX();
        auto const mouseDeltaY = IM::GetMouseDeltaY();

        if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f) {

            auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
            mEulerAngles[1] = mEulerAngles[1] + mouseDeltaX * rotationDistance;    // Reverse for view mat
            mEulerAngles[0] = Math::Clamp(
                mEulerAngles[0] - mouseDeltaY * rotationDistance,
                -45.0f,
                18.0f
            );    // Reverse for view mat

            mIsTransformDirty = true;

        }
    }
    {// Position
        float variantPosition[3];
        mVariant->GetPosition(variantPosition);

        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::GlmRotate(rotationMatrix, mEulerAngles);
    
        glm::vec4 forwardDirection (ForwardVector[0], ForwardVector[1], ForwardVector[2], ForwardVector[3]);

        forwardDirection = forwardDirection * rotationMatrix;
        forwardDirection = glm::normalize(forwardDirection);

        forwardDirection *= mDistance;

        float cameraPosition[3]{
            -variantPosition[0] - forwardDirection[0],
            -variantPosition[1] - forwardDirection[1] + 1.0f,
            -variantPosition[2] - forwardDirection[2]
        };
        ForcePosition(cameraPosition);
    }
    if (mIsTransformDirty) {
        updateTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::OnResize() {
    int32_t width;
    int32_t height;
    RF::GetDrawableSize(width, height);
    MFA_ASSERT(width > 0);
    MFA_ASSERT(height > 0);

    const float ratio = static_cast<float>(width) / static_cast<float>(height);

    Matrix::PreparePerspectiveProjectionMatrix(
        mProjection,
        ratio,
        mFieldOfView,
        mNearPlane,
        mFarPlane
    );

    auto const surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const screenWidth = surfaceCapabilities.currentExtent.width;
    auto const screenHeight = surfaceCapabilities.currentExtent.height;
    RF::WarpMouseInWindow(
        static_cast<int>(static_cast<float>(screenWidth) / 2.0f), 
        static_cast<int>(static_cast<float>(screenHeight) / 2.0f)
    );
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::GetProjection(float outProjectionMatrix[16]) {
    Matrix::CopyGlmToCells(mProjection, outProjectionMatrix);
}

//-------------------------------------------------------------------------------------------------

glm::mat4 const & ThirdPersonCamera::GetProjection() const {
    return mProjection;
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::GetTransform(float outTransformMatrix[16]) {
    Matrix::CopyGlmToCells(mTransform, outTransformMatrix);
}

//-------------------------------------------------------------------------------------------------

glm::mat4 const & ThirdPersonCamera::GetTransform() const {
    return mTransform;
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::ForcePosition(float position[3]) {
    if (IsEqual<3>(mPosition, position) == false) {
        Copy<3>(mPosition, position);
        mIsTransformDirty = true;
    }
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::ForceRotation(float eulerAngles[3]) {
    if (IsEqual<3>(mPosition, eulerAngles) == false) {
        Copy<3>(mPosition, eulerAngles);
        mIsTransformDirty = true;
    }
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::GetPosition(float outPosition[3]) const {
    outPosition[0] = -1 * mPosition[0];
    outPosition[1] = -1 * mPosition[1];
    outPosition[2] = -1 * mPosition[2];
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::GetRotation(float outEulerAngles[3]) const {
    outEulerAngles[0] = mEulerAngles[0];
    outEulerAngles[1] = mEulerAngles[1];
    outEulerAngles[2] = mEulerAngles[2];
}

//-------------------------------------------------------------------------------------------------
// Update transform must be called every frame! Because variant might move.
void ThirdPersonCamera::updateTransform() {

    MFA_ASSERT(mIsTransformDirty == true);

    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::GlmRotate(rotationMatrix, mEulerAngles);

    auto translateMatrix = glm::identity<glm::mat4>();
    Matrix::GlmTranslate(translateMatrix, mPosition);
    
    mTransform = rotationMatrix * translateMatrix;

    mIsTransformDirty = false;

}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::OnUI() {
    UI::BeginWindow(mName.c_str());
    UI::InputFloat3("Position", mPosition);
    UI::InputFloat3("EulerAngles", mEulerAngles);
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------


}
