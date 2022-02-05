#include "ObserverCameraComponent.hpp"

#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/ui_system/UI_System.hpp"

#include "glm/gtx/quaternion.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------
    // TODO We should read position from transform component
    ObserverCameraComponent::ObserverCameraComponent(
        float const fieldOfView,
        float const nearDistance,
        float const farDistance,
        float const moveSpeed,
        float const rotationSpeed
    )
        : CameraComponent(fieldOfView, nearDistance, farDistance)
        , mMoveSpeed(moveSpeed)
        , mRotationSpeed(rotationSpeed)
    {}

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::init()
    {
        CameraComponent::init();

        IM::WarpMouseAtEdges(false);
    }

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Update(float deltaTimeInSec)
    {

        CameraComponent::Update(deltaTimeInSec);

        bool mRotationIsChanged = false;
        if (InputManager::IsLeftMouseDown() == true && UI_System::HasFocus() == false)
        {

            auto const mouseDeltaX = IM::GetMouseDeltaX();
            auto const mouseDeltaY = IM::GetMouseDeltaY();

            if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f)
            {

                auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
                mEulerAngles[1] = mEulerAngles[1] + mouseDeltaX * rotationDistance;    // Reverse for view mat
                mEulerAngles[0] = Math::Clamp(
                    mEulerAngles[0] - mouseDeltaY * rotationDistance,
                    -90.0f,
                    90.0f
                );    // Reverse for view mat

                mRotationIsChanged = true;

            }
        }

        auto const forwardMove = IM::GetForwardMove();
        auto const rightMove = -1.0f * IM::GetRightMove();

        bool mPositionIsChanged = false;
        if (forwardMove != 0.0f || rightMove != 0.0f)
        {
            mPositionIsChanged = true;
        }

        if (mPositionIsChanged == false && mRotationIsChanged == false)
        {
            return;
        }

        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::RotateWithEulerAngle(rotationMatrix, mEulerAngles);

        auto forwardDirection = Math::ForwardVector4;
        forwardDirection = forwardDirection * rotationMatrix;
        forwardDirection = glm::normalize(forwardDirection);

        auto rightDirection = Math::RightVector4;
        rightDirection = rightDirection * rotationMatrix;
        rightDirection = glm::normalize(rightDirection);

        auto const moveDistance = mMoveSpeed * deltaTimeInSec;

        if (forwardMove != 0.0f)
        {
            mPosition[0] = mPosition[0] + forwardDirection[0] * moveDistance * forwardMove;
            mPosition[1] = mPosition[1] + forwardDirection[1] * moveDistance * forwardMove;
            mPosition[2] = mPosition[2] + forwardDirection[2] * moveDistance * forwardMove;
        }
        if (rightMove != 0.0f)
        {
            mPosition[0] = mPosition[0] + rightDirection[0] * moveDistance * rightMove;
            mPosition[1] = mPosition[1] + rightDirection[1] * moveDistance * rightMove;
            mPosition[2] = mPosition[2] + rightDirection[2] * moveDistance * rightMove;
        }

        mIsTransformDirty = true;
    }
    
    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::onUI()
    {
        if(UI::TreeNode("ObserverCamera"))
        {
            CameraComponent::onUI();
            UI::TreePop();            
        }
    }

    //-------------------------------------------------------------------------------------------------

}
