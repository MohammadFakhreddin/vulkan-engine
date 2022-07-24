#include "MeshColliderComponent.hpp"

#include "engine/resource_manager/ResourceManager.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "engine/entity_system/Entity.hpp"

#include <geometry/PxConvexMeshGeometry.h>
#include <geometry/PxTriangleMeshGeometry.h>

namespace MFA
{

    using namespace physx;

    //-------------------------------------------------------------------------------------------------

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

    MeshColliderComponent::MeshColliderComponent(
        bool const isConvex,
        glm::vec3 const & center,
        Physics::SharedHandle<physx::PxMaterial> material
    )
        : ColliderComponent(center, std::move(material))
        , mIsConvex(isConvex)
    {}

    //-------------------------------------------------------------------------------------------------

    void MeshColliderComponent::Init()
    {
        if (mNameId.empty())
        {
            auto const meshRenderer = GetEntity()->GetComponent<MeshRendererComponent>().lock();
            MFA_ASSERT(meshRenderer != nullptr);
            mNameId = meshRenderer->GetNameId();
            MFA_ASSERT(mNameId.empty() == false);
        }

        RC::AcquirePhysicsMesh(
            mNameId,
            mIsConvex,
            [this](Physics::SharedHandle<PxTriangleMesh> const & physicsMesh)->void{
                mPhysicsMesh = physicsMesh;
                ColliderComponent::Init();
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
        PxMeshScale meshScale {Copy<PxVec3>(mScale)};
        return std::make_shared<PxTriangleMeshGeometry>(
            mPhysicsMesh != nullptr ? mPhysicsMesh->Ptr() : nullptr,
            meshScale
        );
    }

    //-------------------------------------------------------------------------------------------------

}
