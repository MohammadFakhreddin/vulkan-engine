#pragma once

#include "Collision.h"
#include "LayerMask.h"

#include "EntitySystem/IEntityComponent.h"

namespace physx
{
    class PxShape;
    class PxMaterial;
    class PxRigidStatic;
}

class RigidbodyComponent;

class ICollisionListener
{
public:
    virtual void onCollision(const Collision & collision) = 0;
};
// TODO: Fill collider component from this class
// Common base class for all collider components.
class Collider : public IEntityComponent
{
public:

    [[nodiscard]]
    EntityEventMask GetEventMask() const override
    {
        return EEntityEvent::Initialize | EEntityEvent::PhysicsCollision;
    }

    void ProcessEvent(const EntityEvent &event) override final;

    /*
    * // We have to use signal class here
    IsTrigger()
    OnCollisionEnter(Collision collision)
    OnCollisionExit(Collision collision)
    OnCollisionStay(Collision collision)
    OnTriggerEnter(Collider other)
    OnTriggerExit(Collider other)
    OnTriggerStay(Collider other)
    */
    [[nodiscard]] physx::PxShape *GetShape() const noexcept { return mShape; }

    [[nodiscard]] ComponentEventPriority GetEventPriority() const noexcept final
    {
        return static_cast<ComponentEventPriority>(10);
    }

    /*
     * Assigns layer mask for raycast filtering
     * This function must be called when creating component, otherwise it won't have any effect
     */
    void SetLayerMask(const LayerMask & layerMask);

    [[nodiscard]]
    const LayerMask & GetLayerMask() const noexcept;

    [[nodiscard]]
    physx::PxRigidStatic *GetStaticRigidActor() const noexcept
    {
        return mStaticRigidActor;
    }

    uint16_t RegisterCollisionListener(ICollisionListener * classPtr);

    void RemoveCollisionListener(uint16_t id);

    
    // TODO Center is not used anywhere
    void SetCenter(const glm::vec3 &center)
    {
        mCenter = center;
    }

    [[nodiscard]] glm::vec3 GetCenter() const noexcept
    {
        return mCenter;
    }
    
protected:

    /**
     * \return Returns true if there is need for baseCollider to generate staticRigidActor
     */
    [[nodiscard]]
    virtual bool init() = 0;

    virtual void update(float deltaTimeInSec) {}

    virtual void shutdown() {};

    physx::PxMaterial *mMaterial = nullptr;             // The material used by the collider. If material is shared by colliders, it will duplicate the material and assign it to the collider.

    bool mIsTrigger {};			                        // A trigger doesn't register a collision with an incoming Rigidbody. Instead, it sends OnTriggerEnter, OnTriggerExit and OnTriggerStay 
                                                        // message when a rigidbody enters or exits the trigger volume.

    RigidbodyComponent *mAttachedRigidbody = nullptr;	// The rigidbody the collider is attached to. Returns null if the collider is attached to no rigidbody.
                                                        // Colliders are automatically connected to the rigidbody attached to the same game object or attached to any parent game object.

    physx::PxRigidStatic *mStaticRigidActor = nullptr;  // Objects without rigidbody-component contain

    physx::PxShape *mShape = nullptr;

    // TODO use center
    glm::vec3 mCenter {};                               // The center of the sphere in the object's local space.
    

private:

    void baseInit();

    void handleCollisionEvent(const Collision & collision);

    LayerMask mLayerMask {};

    struct Listener
    {
        Listener(const uint16_t id_, ICollisionListener * classPtr_)
            : id(id_)
            , classPtr(classPtr_)
        {}
        uint16_t id = 0;
        ICollisionListener * classPtr = nullptr;
    };
    std::vector<Listener> mCollisionListeners {};
    uint16_t mNextListenerId = 0;

};

