#include "ThirdPersonCameraComponent.hpp"

#include "engine/InputManager.hpp"
#include "engine/ui_system/UI_System.hpp"
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
        std::shared_ptr<TransformComponent> const & followTarget,
        float const fieldOfView,
        float const nearDistance,
        float const farDistance,
        float const rotationSpeed,
        float const distance,
        bool const wrapMouseAtEdges,
        glm::vec3 const extraOffset
    )
        : CameraComponent(fieldOfView, nearDistance, farDistance)
        , mFollowTarget(followTarget)
        , mDistance(distance)
        , mRotationSpeed(rotationSpeed)
        , mWrapMouseAtEdges(wrapMouseAtEdges)
        , mExtraOffset(extraOffset)
    {
        MFA_ASSERT(followTarget != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Init()
    {
        CameraComponent::Init();

        if (auto const followTarget = mFollowTarget.lock())
        {
            mFollowTargetListenerId = followTarget->RegisterChangeListener([this]()->void
            {
                OnFollowTargetMove();
            });
        }

        OnWrapMouseAtEdgesChange();
        OnFollowTargetMove();
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Update(float const deltaTimeInSec)
    {
        CameraComponent::Update(deltaTimeInSec);

        auto const transform = mTransformComponent.lock();
        if (transform == nullptr)
        {
            return;
        }

        // Checking if rotation is changed
        auto const mouseDeltaX = IM::GetMouseDeltaX();
        auto const mouseDeltaY = IM::GetMouseDeltaY();

        if (mouseDeltaX == 0.0f && mouseDeltaY == 0.0f)
        {
            return;
        }

        auto eulerAngles = transform->GetLocalRotation().GetEulerAngles();
        auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
        // TODO: I could have multiplied some extra quaternion as well
        eulerAngles.y = eulerAngles.y + rotationDistance * mouseDeltaX;    // Reverse for view mat
        eulerAngles.x = Math::Clamp(
            eulerAngles.x - rotationDistance * mouseDeltaY,
            -45.0f,
            18.0f
        );    // Reverse for view mat

        transform->UpdateLocalRotation(eulerAngles);
        OnFollowTargetMove();
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::Shutdown()
    {
        CameraComponent::Shutdown();
        if (auto const ptr = mFollowTarget.lock())
        {
            ptr->UnRegisterChangeListener(mFollowTargetListenerId);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::OnUI()
    {
        // TODO: Try to generate ui automatically
        if(UI::TreeNode("ThirdPersonCamera"))
        {
            CameraComponent::OnUI();

            UI::InputFloat("Rotation speed:", mRotationSpeed);

            bool positionNeedChanged = false;

            positionNeedChanged |= UI::InputFloat("Distance", mDistance);

            positionNeedChanged |= UI::InputFloat("Extra offset", mExtraOffset);

            if (positionNeedChanged)
            {
                OnFollowTargetMove();
            }

            if (UI::Checkbox("Wrap mouse at edges", mWrapMouseAtEdges))
            {
                OnWrapMouseAtEdgesChange();
            }

            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::WrapMouseAtEdges(bool const wrap)
    {
        mWrapMouseAtEdges = wrap;
        IM::WarpMouseAtEdges(mWrapMouseAtEdges);
    }

    //-------------------------------------------------------------------------------------------------

    bool ThirdPersonCameraComponent::WrapMouseAtEdges() const
    {
        return mWrapMouseAtEdges;
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::SetFollowTarget(std::shared_ptr<TransformComponent> const& followTarget)
    {
        if (auto const oldFollowTarget = mFollowTarget.lock())
        {
            oldFollowTarget->UnRegisterChangeListener(mFollowTargetListenerId);
        }
        mFollowTarget = followTarget;
        if (auto const newFollowTarget = mFollowTarget.lock())
        {
            mFollowTargetListenerId = newFollowTarget->RegisterChangeListener([this]()->void
            {
                OnFollowTargetMove();
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::OnFollowTargetMove() const
    {
        auto const followTarget = mFollowTarget.lock();
        if (followTarget == nullptr)
        {
            return;
        }
        auto const myTransform = mTransformComponent.lock();
        if (myTransform == nullptr)
        {
            return;
        }

        glm::vec3 const followPosition = followTarget->GetWorldPosition();

        auto const vector = mForward * mDistance;

        // The extra vector is used to increase camera height
        auto const position = followPosition - vector + mExtraOffset;

        myTransform->UpdateLocalPosition(position);
    }


    //-------------------------------------------------------------------------------------------------

    void ThirdPersonCameraComponent::OnWrapMouseAtEdgesChange() const
    {
        IM::WarpMouseAtEdges(mWrapMouseAtEdges);
    }

    //-------------------------------------------------------------------------------------------------

}
