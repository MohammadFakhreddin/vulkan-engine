#include "CapsuleColliderComponent.hpp"

#include <geometry/PxCapsuleGeometry.h>

#include "engine/ui_system/UI_System.hpp"

namespace MFA
{

    using namespace physx;

    //-------------------------------------------------------------------------------------------------

    CapsuleColliderComponent::CapsuleColliderComponent(
        float const halfHeight,
        float const radius,
        glm::vec3 const & center,
        Physics::SharedHandle<physx::PxMaterial> material
    )
        : ColliderComponent(center, std::move(material))
        , mHalfHeight(halfHeight)
        , mRadius(radius)
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

            bool needShapeUpdate = false;

            needShapeUpdate |= UI::InputFloat<1>("Half height", mHalfHeight);
            needShapeUpdate |= UI::InputFloat<1>("Radius", mRadius);
            
            if (needShapeUpdate)
            {
                UpdateShapeGeometry();
            }

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

        PxCapsuleGeometry geometry {mRadius, mHalfHeight};
        std::vector<std::shared_ptr<PxGeometry>> geometries {std::make_shared<PxCapsuleGeometry>(geometry)};
        // TODO: We need direction to set local pos quaternion
        return geometries;
    }

    //-------------------------------------------------------------------------------------------------
}
