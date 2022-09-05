#include "BoxColliderComponent.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/physics/Physics.hpp"
#include "engine/ui_system/UI_System.hpp"

#include <physx/PxRigidActor.h>

namespace MFA
{
    using namespace physx;

    //-------------------------------------------------------------------------------------------------

    BoxColliderComponent::BoxColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    BoxColliderComponent::~BoxColliderComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void BoxColliderComponent::OnUI()
    {
        if (UI::TreeNode(Name))
        {
            Parent::OnUI();
            glm::vec3 size = mHalfSize * 2.0f;
            if (UI::InputFloat("Size", size))
            {
                mHalfSize = size * 0.5f;
                UpdateShapeGeometry();
            }
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoxColliderComponent::Clone(Entity * entity) const
    {
        Parent::Clone(entity);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void BoxColliderComponent::Serialize(nlohmann::json & jsonObject) const
    {
        Parent::Serialize(jsonObject);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void BoxColliderComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        Parent::Deserialize(jsonObject);
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }
    
    //-------------------------------------------------------------------------------------------------

    std::vector<std::shared_ptr<PxGeometry>> BoxColliderComponent::ComputeGeometry()
    {
        MFA_ASSERT(mHalfSize.x > 0.0f);
        MFA_ASSERT(mHalfSize.y > 0.0f);
        MFA_ASSERT(mHalfSize.z > 0.0f);

        auto const halfSize = mHalfSize * mScale;
        std::vector<std::shared_ptr<PxGeometry>> geometries {
            std::make_shared<PxBoxGeometry>(Copy<PxVec3>(halfSize))
        };
        return geometries;
    }

    //-------------------------------------------------------------------------------------------------

}
