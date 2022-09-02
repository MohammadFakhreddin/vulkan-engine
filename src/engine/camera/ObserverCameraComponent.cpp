#include "ObserverCameraComponent.hpp"

#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/ui_system/UI_System.hpp"

#include "glm/gtx/quaternion.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    ObserverCameraComponent::ObserverCameraComponent(
        float const fieldOfView,
        float const nearDistance,
        float const farDistance,
        float const moveSpeed,
        float const rotationSpeed
    )
        : Parent(fieldOfView, nearDistance, farDistance)
        , mMoveSpeed(moveSpeed)
        , mRotationSpeed(rotationSpeed)
    {}

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Init()
    {
        Parent::Init();

        IM::WarpMouseAtEdges(false);
    }

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Update(float const deltaTimeInSec)
    {
        Parent::Update(deltaTimeInSec);

        auto const transform = mTransformComponent.lock();
        if (transform == nullptr)
        {
            return;
        }

        auto eulerAngles = transform->GetLocalRotation().GetEulerAngles();
        auto position = transform->GetLocalPosition();

        if (InputManager::IsLeftMouseDown() == true && UI_System::HasFocus() == false)
        {

            auto const mouseDeltaX = IM::GetMouseDeltaX();
            auto const mouseDeltaY = IM::GetMouseDeltaY();

            if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f)
            {
                eulerAngles = transform->GetLocalRotation().GetEulerAngles();
                auto const rotationDistance = mRotationSpeed * deltaTimeInSec;
                eulerAngles.y = eulerAngles.y + rotationDistance * mouseDeltaX;    // Reverse for view mat
                eulerAngles.x = Math::Clamp(
                    eulerAngles.x - rotationDistance * mouseDeltaY,
                    -90.0f,
                    90.0f
                );    // Reverse for view mat
            }
        }

        auto const forwardMove = IM::GetForwardMove();
        auto const rightMove = IM::GetRightMove();

        if (forwardMove != 0.0f || rightMove != 0.0f)
        {
            auto const moveDistance = mMoveSpeed * deltaTimeInSec;

            if (forwardMove != 0.0f)
            {
                position = position + mForward * moveDistance * forwardMove;
            }
            if (rightMove != 0.0f)
            {
                position = position + mRight * moveDistance * rightMove;
            }
        }

        transform->SetLocalTransform(position, eulerAngles, transform->GetLocalScale());
    }
    
    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::OnUI()
    {
        if(UI::TreeNode("ObserverCamera"))
        {
            CameraComponent::OnUI();
            UI::TreePop();            
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Serialize(nlohmann::json & jsonObject) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void ObserverCameraComponent::Clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

}
