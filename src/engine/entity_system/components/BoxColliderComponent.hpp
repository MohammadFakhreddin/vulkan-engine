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
            EventTypes::EmptyEvent,
            ColliderComponent
        )

        explicit BoxColliderComponent();

        ~BoxColliderComponent() override;

        void OnUI() override;
        
        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    protected:

        [[nodiscard]]
        std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() override;

    private:

        MFA_VECTOR_VARIABLE2(HalfSize, glm::vec3, glm::vec3 {}, UpdateShapeGeometry)
    };

    using BoxCollider = BoxColliderComponent;

}
