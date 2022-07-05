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
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )

        explicit BoxColliderComponent(glm::vec3 size, glm::vec3 center);

        ~BoxColliderComponent() override;

        void Init() override;

        void Shutdown() override;

        void OnUI() override;

        void OnTransformChange() override;

    protected:
        
        physx::PxTransform ComputePxTransform() override;

    private:

        void CreateShape();

        glm::vec3 mHalfSize {};
        glm::vec4 mCenter {};

    };

}
