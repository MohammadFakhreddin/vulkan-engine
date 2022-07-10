#include "MeshColliderComponent.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    MeshColliderComponent::MeshColliderComponent(
        std::string const & nameId,
        bool const isConvex,
        glm::vec3 const & center
    )
        : ColliderComponent(center)
        , mNameId(nameId)
        , mIsConvex(isConvex)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Init()
    {
        ColliderComponent::Init();
    }
    
    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Serialize(nlohmann::json & jsonObject) const
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Deserialize(nlohmann::json const & jsonObject)
    {}
    
    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Clone(Entity * entity) const
    {}
    
    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<physx::PxGeometry> MeshColliderComponent::ComputeGeometry()
    {
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------

}
