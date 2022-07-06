#pragma once

#include "ColliderComponent.hpp"
#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace MFA
{

    class BoxColliderComponent : public ColliderComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            BoxColliderComponent,
            FamilyType::Collider,
            EventTypes::EmptyEvent,
            ColliderComponent
        )

        explicit BoxColliderComponent(glm::vec3 size, glm::vec3 center);

        ~BoxColliderComponent() override;

        void Init() override;

        void Shutdown() override;

        void OnUI() override;

        void OnTransformChange() override;

    protected:
        
        physx::PxTransform ComputePxTransform() override;

        void ComputeTransform(
            physx::PxTransform const & pxTransform,
            glm::vec3 & outPosition,
            glm::quat & outRotation
        ) override;

    private:

        void CreateShape();

        glm::vec3 mHalfSize {};
        glm::vec4 mCenter {};

    };

}
