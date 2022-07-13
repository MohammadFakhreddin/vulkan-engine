#include "MeshColliderComponent.hpp"

#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/physics/Physics.hpp"

#include <cooking/PxConvexMeshDesc.h>
#include <geometry/PxConvexMeshGeometry.h>

#include "TransformComponent.hpp"

namespace MFA
{

    using namespace physx;

    //-------------------------------------------------------------------------------------------------
    // TODO: We need to cache mesh colliders
    MeshColliderComponent::MeshColliderComponent(
        std::string nameId,
        bool const isConvex,
        glm::vec3 const & center,
        Physics::SharedHandle<PxMaterial> material
    )
        : ColliderComponent(center, std::move(material))
        , mNameId(std::move(nameId))
        , mIsConvex(isConvex)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Init()
    {
        ColliderComponent::Init();

        RC::AcquirePhysicsMesh(
            mNameId,
            mIsConvex,
            [this](Physics::SharedHandle<PxConvexMesh> const & physicsMesh)->void{
                mPhysicsMesh = physicsMesh;
                UpdateShapeGeometry();
            }
        );
    }
    
    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Serialize(nlohmann::json & jsonObject) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }

    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }
    
    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Clone(Entity * entity) const
    {
        MFA_NOT_IMPLEMENTED_YET("MFA");
    }
    
    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<PxGeometry> MeshColliderComponent::ComputeGeometry()
    {
        if (mPhysicsMesh == nullptr)
        {
            return nullptr;
        }
        
        PxMeshScale meshScale {Copy<PxVec3>(mScale)};
        return std::make_shared<PxConvexMeshGeometry>(mPhysicsMesh->Ptr(), meshScale);
    }

    //-------------------------------------------------------------------------------------------------

}
