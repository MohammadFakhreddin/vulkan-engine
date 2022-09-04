#pragma once

#include "ColliderComponent.hpp"
#include "engine/entity_system/Component.hpp"

#include <string>

namespace MFA
{
    class MeshColliderComponent final : public ColliderComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            MeshColliderComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            ColliderComponent
        )

        explicit MeshColliderComponent(bool isConvex);

        explicit MeshColliderComponent(std::string nameId, bool isConvex);

        void Init() override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Clone(Entity * entity) const override;

    protected:

        std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() override;

    private:

        std::string mNameId {};
        bool mIsConvex = false;
        std::shared_ptr<physx::PxGeometry> mGeometry {};
        std::shared_ptr<Physics::TriangleMeshGroup> mPhysicsMesh {};
    };

    using MeshCollider = MeshColliderComponent;

}
