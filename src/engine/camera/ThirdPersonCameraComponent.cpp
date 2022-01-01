#include "ThirdPersonCameraComponent.hpp"

#include "engine/InputManager.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
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

    void ThirdPersonCameraComponent::init()
    {
        CameraComponent::init();

        mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
        MFA_ASSERT(mTransformComponent.expired() == false);

        // TODO: Should camera use transform component for its stored values ?
        mTransformChangeListenerId = mTransformComponent.lock()->RegisterChangeListener([this]()->void
        {
            mIsTransformDirty = true;
        });

        IM::WarpMouseAtEdges(true);
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Update(float deltaTimeInSec, RT::CommandRecordState const & recordState)
    {
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
            if (auto const transformComponentPtr = mTransformComponent.lock()) {
                auto const variantPosition = transformComponentPtr->GetWorldPosition();

                auto rotationMatrix = glm::identity<glm::mat4>();
                Matrix::Rotate(rotationMatrix, mEulerAngles);

                glm::vec4 forwardDirection = Math::ForwardVector;

                forwardDirection = forwardDirection * rotationMatrix;
                forwardDirection = glm::normalize(forwardDirection);

                forwardDirection *= mDistance;

                mPosition[0] = -variantPosition[0] - forwardDirection[0];
                mPosition[1] = -variantPosition[1] - forwardDirection[1] + 1.0f;
                mPosition[2] = -variantPosition[2] - forwardDirection[2];
            }
        }

        CameraComponent::Update(deltaTimeInSec, recordState);

    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::shutdown()
    {
        CameraComponent::shutdown();
        if (auto const ptr = mTransformComponent.lock())
        {
            ptr->UnRegisterChangeListener(mTransformChangeListenerId);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::onUI()
    {
        if(UI::TreeNode("ThirdPersonCamera"))
        {
            CameraComponent::onUI();
            UI::TreePop();            
        }
    }

    //-------------------------------------------------------------------------------------------------

}
