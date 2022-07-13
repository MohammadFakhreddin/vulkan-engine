#pragma once

#include "ColliderComponent.hpp"
#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>

namespace MFA
{

    class BoxColliderComponent final : public ColliderComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            BoxColliderComponent,
            FamilyType::Collider,
            EventTypes::EmptyEvent,
            ColliderComponent
        )

        explicit BoxColliderComponent(
            glm::vec3 const & size,
            glm::vec3 const & center,
            Physics::SharedHandle<physx::PxMaterial> material
        );

        ~BoxColliderComponent() override;

        void OnUI() override;

        void OnTransformChange() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        [[nodiscard]]
        std::shared_ptr<physx::PxGeometry> ComputeGeometry() override;

    private:

        glm::vec3 mHalfSize {};
    
    };

}
