#include "ThirdPersonCamera.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockCommon.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

ThirdPersonCamera::ThirdPersonCamera(
    float const fieldOfView,
    float const farPlane,
    float const nearPlane,
    DrawableVariant * variant,
    float distance[3], 
    float eulerAngles[3]
)
    : mFieldOfView(fieldOfView)
    , mFarPlane(farPlane)
    , mNearPlane(nearPlane)
    , mVariant(variant)
{
    MFA_ASSERT(mVariant != nullptr);
    Copy<3>(mRelativePosition, distance);
    Copy<3>(mRelativeEulerAngles, eulerAngles);
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::Init() {
    OnResize();
    updateTransform();
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::OnUpdate(float deltaTime) {
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
    if (IsEqual<3>(mRelativePosition, position) == false) {
        Copy<3>(mRelativePosition, position);
        mIsTransformDirty = true;
    }
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::ForceRotation(float eulerAngles[3]) {
    if (IsEqual<3>(mRelativePosition, eulerAngles) == false) {
        Copy<3>(mRelativePosition, eulerAngles);
        mIsTransformDirty = true;
    }
}

//-------------------------------------------------------------------------------------------------

void ThirdPersonCamera::GetPosition(float outPosition[3]) const {
    auto transform = mVariant->GetTransform();

    const auto variantX = transform[0][3];
    const auto variantY = transform[1][3];
    const auto variantZ = transform[2][3];

    outPosition[0] = -1 * (variantX + mRelativePosition[0]);
    outPosition[1] = -1 * (variantY + mRelativePosition[1]);
    outPosition[2] = -1 * (variantZ + mRelativePosition[2]);
}

//-------------------------------------------------------------------------------------------------
// Update transform must be called every frame! Because variant might move.
void ThirdPersonCamera::updateTransform() {

    MFA_ASSERT(mIsTransformDirty == true);

    mTransform = mVariant->GetTransform();

    // TODO Maybe it is better to rotate on our own
    Matrix::GlmRotate(mTransform, mRelativeEulerAngles);

    Matrix::GlmTranslate(mTransform, mRelativePosition);

    mIsTransformDirty = false;

}

//-------------------------------------------------------------------------------------------------

}
