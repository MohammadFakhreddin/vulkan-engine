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
        Physics::SharedHandle<PxMaterial> material
    )
        : ColliderComponent(
            {},
            {},
            std::move(material))
        , mHalfHeight(halfHeight)
        , mRadius(radius)
        , mCapsuleDirection(direction)
    {
        ComputeCenter();
        RotateToDirection();
        MFA_ASSERT(halfHeight > 0.0f);
        MFA_ASSERT(radius > 0.0f);
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

    void CapsuleColliderComponent::RotateToDirection()
    {
        glm::vec3 axis;
        if (mCapsuleDirection == CapsuleDirection::X)
        {
            axis = Math::UpVec3;
        }
        else if (mCapsuleDirection == CapsuleDirection::Y)
        {
            axis = Math::ForwardVec3;
        }
        else if (mCapsuleDirection == CapsuleDirection::Z)
        {
            axis = Math::RightVec3;
        }
        else
        {
            MFA_ASSERT(false);
        }
        mRotation = glm::angleAxis(Math::PiFloat * 0.5f, axis);
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
            ComputeCenter();
            UpdateShapeRelativeTransform();
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
            RotateToDirection();
            UpdateShapeRelativeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void CapsuleColliderComponent::ComputeCenter()
    {
        float height = 0.0f;
        if (mCapsuleDirection == CapsuleDirection::Y)
        {
            height = mHalfHeight + mRadius;
        }
        else
        {
            height = mRadius;
        }

        mCenter = Math::UpVec3 * height;
    }

    //-------------------------------------------------------------------------------------------------

}
