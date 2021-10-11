#include "ThirdPersonCameraComponent.hpp"

#include "engine/InputManager.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    ThirdPersonCameraComponent::ThirdPersonCameraComponent(
        float const fieldOfView,
        float const nearDistance,
        float const farDistance,
        float const rotationSpeed
    )
        : CameraComponent(fieldOfView, nearDistance, farDistance)
        , mRotationSpeed(rotationSpeed)
    {}

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::SetDistanceAndRotation(
        float const distance,
        float eulerAngles[3]
    )
    {
        MFA_ASSERT(distance >= 0);
        mDistance = distance;

        Matrix::CopyCellsToGlm(eulerAngles, mEulerAngles);

        mIsTransformDirty = true;
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Init()
    {
        CameraComponent::Init();

        mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
        MFA_ASSERT(mTransformComponent != nullptr);

        mTransformChangeListenerId = mTransformComponent->RegisterChangeListener([this]()->void
    {
            mIsTransformDirty = true;
        });

        OnResize();
        updateTransform();

        IM::WarpMouseAtEdges(true);

    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Update(float const deltaTimeInSec)
    {
        CameraComponent::Update(deltaTimeInSec);

        // Checking if rotation is changed
        auto const mouseDeltaX = IM::GetMouseDeltaX();
        auto const mouseDeltaY = IM::GetMouseDeltaY();

        if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f)
        {

            auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
            mEulerAngles[1] = mEulerAngles[1] + mouseDeltaX * rotationDistance;    // Reverse for view mat
            mEulerAngles[0] = Math::Clamp(
                mEulerAngles[0] - mouseDeltaY * rotationDistance,
                -45.0f,
                18.0f
            );    // Reverse for view mat

            mIsTransformDirty = true;
        }

        if (mIsTransformDirty)
        {
            float variantPosition[3];
            mTransformComponent->GetPosition(variantPosition);

            auto rotationMatrix = glm::identity<glm::mat4>();
            Matrix::Rotate(rotationMatrix, mEulerAngles);

            glm::vec4 forwardDirection = ForwardVector;

            forwardDirection = forwardDirection * rotationMatrix;
            forwardDirection = glm::normalize(forwardDirection);

            forwardDirection *= mDistance;

            mPosition[0] = -variantPosition[0] - forwardDirection[0];
            mPosition[1] = -variantPosition[1] - forwardDirection[1] + 1.0f;
            mPosition[2] = -variantPosition[2] - forwardDirection[2];

            updateTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Shutdown()
    {
        CameraComponent::Shutdown();
        mTransformComponent->UnRegisterChangeListener(mTransformChangeListenerId);
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::GetTransform(float outTransformMatrix[16])
    {
        Matrix::CopyGlmToCells(mTransform, outTransformMatrix);
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 const & ThirdPersonCameraComponent::GetTransform() const
    {
        return mTransform;
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::ForcePosition(float position[3])
    {
        if (Matrix::IsEqual(mPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mPosition);
            mIsTransformDirty = true;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::ForceRotation(float eulerAngles[3])
    {
        if (Matrix::IsEqual(mPosition, eulerAngles) == false)
        {
            Matrix::CopyCellsToGlm(eulerAngles, mEulerAngles);
            mIsTransformDirty = true;
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::GetPosition(float outPosition[3]) const
    {
        outPosition[0] = -1 * mPosition[0];
        outPosition[1] = -1 * mPosition[1];
        outPosition[2] = -1 * mPosition[2];
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::GetRotation(float outEulerAngles[3]) const
    {
        outEulerAngles[0] = mEulerAngles[0];
        outEulerAngles[1] = mEulerAngles[1];
        outEulerAngles[2] = mEulerAngles[2];
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::updateTransform()
    {

        MFA_ASSERT(mIsTransformDirty == true);

        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::Rotate(rotationMatrix, mEulerAngles);

        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mPosition);

        mTransform = rotationMatrix * translateMatrix;

        mIsTransformDirty = false;

    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::OnUI()
    {
        UI::BeginWindow(GetEntity()->GetName().c_str());
        UI::InputFloat3("Position", reinterpret_cast<float *>(&mPosition));
        UI::InputFloat3("EulerAngles", reinterpret_cast<float *>(&mEulerAngles));
        CameraComponent::OnUI();
        UI::EndWindow();
    }

    //-------------------------------------------------------------------------------------------------

}
