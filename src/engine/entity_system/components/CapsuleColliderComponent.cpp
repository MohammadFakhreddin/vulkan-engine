#include "CapsuleColliderComponent.hpp"

#include "engine/ui_system/UI_System.hpp"
#include "engine/BedrockMatrix.hpp"

#include <geometry/PxCapsuleGeometry.h>

namespace MFA
{

    using namespace physx;

    //-------------------------------------------------------------------------------------------------

    CapsuleColliderComponent::CapsuleColliderComponent(
        float const halfHeight,
        float const radius,
        CapsuleDirection const direction,
        glm::vec3 const & center,
        Physics::SharedHandle<PxMaterial> material
    )
        : ColliderComponent(center, RotateToDirection(direction), std::move(material))
        , mHalfHeight(halfHeight)
        , mRadius(radius)
        , mCapsuleDirection(direction)
    {
        MFA_ASSERT(halfHeight > 0.0f);
        MFA_ASSERT(radius >= 0.0f);
    }

    //-------------------------------------------------------------------------------------------------

    CapsuleColliderComponent::~CapsuleColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::OnUI()
    {
        if (UI::TreeNode(Name))
        {
            Parent::OnUI();

            UI_ShapeUpdate();

            UI_CapsuleDirection();
            
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::OnTransformChange()
    {
        Parent::OnTransformChange();
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::Clone(Entity * entity) const
    {
        Parent::Clone(entity);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::Serialize(nlohmann::json & jsonObject) const
    {
        Parent::Serialize(jsonObject);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        Parent::Deserialize(jsonObject);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    [[nodiscard]]
    std::vector<std::shared_ptr<PxGeometry>> CapsuleColliderComponent::ComputeGeometry()
    {

        PxCapsuleGeometry geometry {mRadius * mScale.x * mScale.z, mHalfHeight * mScale.y};
        std::vector<std::shared_ptr<PxGeometry>> geometries {std::make_shared<PxCapsuleGeometry>(geometry)};
        return geometries;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat CapsuleColliderComponent::RotateToDirection(CapsuleDirection const direction)
    {
        glm::vec3 axis;
        if (direction == CapsuleDirection::X)
        {
            axis = Math::UpVec3;
        }
        else if (direction == CapsuleDirection::Y)
        {
            axis = Math::ForwardVec3;
        }
        else if (direction == CapsuleDirection::Z)
        {
            axis = Math::RightVec3;
        }
        else
        {
            MFA_ASSERT(false);
        }
        return glm::angleAxis(Math::PiFloat * 0.5f, axis);
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::UI_ShapeUpdate()
    {
        bool needShapeUpdate = false;

        needShapeUpdate |= UI::InputFloat<1>("Half height", mHalfHeight);
        needShapeUpdate |= UI::InputFloat<1>("Radius", mRadius);

        if (needShapeUpdate)
        {
            UpdateShapeGeometry();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::UI_CapsuleDirection()
    {
        std::vector<std::string> items{ "X", "Y", "Z" };
        int selectedItem = static_cast<int>(mCapsuleDirection);
        if (UI::Combo("Capsule direction", &selectedItem, items))
        {
            mCapsuleDirection = static_cast<CapsuleDirection>(selectedItem);
            mRotation = RotateToDirection(mCapsuleDirection);
            UpdateShapeRelativeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

}
