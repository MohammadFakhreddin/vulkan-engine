#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/physics/PhysicsTypes.hpp"

#include "TransformComponent.hpp"
#include "engine/BedrockRotation.hpp"

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <physx/geometry/PxGeometry.h>

// TODO: Use this link
// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html

namespace physx
{
    class PxMaterial;
    class PxShape;
    class PxRigidActor;
    class PxActor;
    class PxRigidDynamic;
    class PxRigidStatic;
}

namespace MFA
{
    // TODO: Layer and layerMask
 
    class ColliderComponent : public Component
    {
    public:

        MFA_ABSTRACT_COMPONENT_PROPS(
            ColliderComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::ActivationChangeEvent,
            Component
        )

        explicit ColliderComponent();

        ~ColliderComponent() override;

        void Init() override;

        void Shutdown() override;

        void OnUI() override;

        [[nodiscard]]
        Physics::SharedHandle<physx::PxRigidDynamic> GetRigidDynamic() const;

        void OnActivationStatusChanged(bool isActive) override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        void Clone(Entity * entity) const override;

    protected:

        virtual void OnTransformChange(Transform::ChangeParams const & params);

        void CreateShape();

        void UpdateShapeRelativeTransform() const;

        void UpdateShapeGeometry();

        [[nodiscard]]
        virtual std::vector<std::shared_ptr<physx::PxGeometry>> ComputeGeometry() = 0;

    private:

        void CreateShape(std::vector<std::shared_ptr<physx::PxGeometry>> const & geometries);

        void RemoveShapes();

        void OnMaterialChange() const;

    protected:

        std::weak_ptr<TransformComponent> mTransform;

        physx::PxRigidActor * mActor = nullptr;
        // Either one of this 2 are available
        Physics::SharedHandle<physx::PxRigidDynamic> mRigidDynamic = nullptr;
        Physics::SharedHandle<physx::PxRigidStatic> mRigidStatic = nullptr;

        std::vector<physx::PxShape *> mShapes{};

        SignalId mTransformChangeListenerId = -1;

        bool mIsDynamic = false;

        MFA_ATOMIC_VARIABLE2(Center, glm::vec3, {}, UpdateShapeRelativeTransform)

    protected: 

        MFA_ATOMIC_VARIABLE2(Rotation, Rotation, Rotation {}, UpdateShapeRelativeTransform)

        MFA_ATOMIC_VARIABLE2(Material, Physics::SharedHandle<physx::PxMaterial>, nullptr, OnMaterialChange)
        
        // Global world scale
        glm::vec3 mScale{};

        bool mInitialized = false;

    };

    using Collider = ColliderComponent;

}
