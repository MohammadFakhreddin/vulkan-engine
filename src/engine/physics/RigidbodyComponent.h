#pragma once

#include "EntitySystem/IEntityComponent.h"

#include <glm/fwd.hpp>

namespace physx
{
    class PxRigidActor;
}

class BitStreamDelta;
class TransformComponent;

class RigidbodyComponent final : public IEntityComponent
{
public:

    [[nodiscard]]
    EntityEventMask GetEventMask() const override
    {
        return EEntityEvent::Update | EEntityEvent::Initialize;
    }

    void ProcessEvent(const EntityEvent &event) override;

    void AddExplosionForce();	        // Applies a force to a rigidbody that simulates explosion effects.
    void AddForce();	                // Adds a force to the Rigidbody.
    void AddForceAtPosition();	        // Applies force at position.As a result this will apply a torque andforce on the object.
    void AddRelativeForce();	        // Adds a force to the rigidbody relative to its coordinate system.
    void AddRelativeTorque();	        // Adds a torque to the rigidbody relative to its coordinate system.
    void AddTorque();	                // Adds a torque to the rigidbody.
    void ClosestPointOnBounds();	    // The closest point to the bounding box of the attached colliders.
    void GetPointVelocity();	        // The velocity of the rigidbody at the point worldPoint in global space.
    void GetRelativePointVelocity();	// The velocity relative to the rigidbody at the point relativePoint.
    void IsSleeping();	                // Is the rigidbody sleeping ?
    void MovePosition();	            // Moves the kinematic Rigidbody towards position.
    void MoveRotation();	            // Rotates the rigidbody to rotation.
    void ResetCenterOfMass();	        // Reset the center of mass of the rigidbody.
    void ResetInertiaTensor();	        // Reset the inertia tensor value and rotation.
    void SetDensity();	                // Sets the mass based on the attached colliders assuming a constant density.
    void Sleep();	                    // Forces a rigidbody to sleep at least one frame.
    void SweepTest();	                // Tests if a rigidbody would collide with anything, if it was moved through the Scene.
    void SweepTestAll();	            // Like Rigidbody.SweepTest, but returns all hits.
    void WakeUp();	                    // Forces a rigidbody to wake up.

    void SetIsKinematic(bool isKinematic);
    [[nodiscard]] bool IsKinematic() const noexcept;

    void SetOrientation(const glm::quat & orientation) const;

    [[nodiscard]]
    glm::vec3 GetVelocity() const;      // The velocity vector of the rigidbody.It represents the rate of change of Rigidbody position.
    void SetVelocity(glm::vec3 velocity);

    [[nodiscard]]
    float GetAngularDrag() const noexcept;  // The angular drag (damping) of the object.
    void SetAngularDrag(const float angularDrag);

    [[nodiscard]]
    physx::PxRigidActor * GetActor() const noexcept
    {
        return mActor;
    }

private:

    void init();
    void updateTransformFromActor(bool & returns) const;

    void update() const;

    void updateTransformFromActor() const;

    /*
    centerOfMass	The center of mass relative to the transform's origin.
    collisionDetectionMode	The Rigidbody's collision detection mode.
    constraints	Controls which degrees of freedom are allowed for the simulation of this Rigidbody.
    detectCollisions	Should collision detection be enabled? (By default always enabled).
    drag	The drag of the object.
    freezeRotation	Controls whether physics will change the rotation of the object.
    inertiaTensor	The diagonal inertia tensor of mass relative to the center of mass.
    inertiaTensorRotation	The rotation of the inertia tensor.
    interpolation	Interpolation allows you to smooth out the effect of running physics at a fixed frame rate.
    mass	The mass of the rigidbody.
    maxAngularVelocity	The maximimum angular velocity of the rigidbody measured in radians per second. (Default 7) range { 0, infinity }.
    maxDepenetrationVelocity	Maximum velocity of a rigidbody when moving out of penetrating state.
    position	The position of the rigidbody.
    rotation	The rotation of the Rigidbody.
    sleepThreshold	The mass-normalized energy threshold, below which objects start going to sleep.
    solverIterations	The solverIterations determines how accurately Rigidbody joints and collision contacts are resolved. Overrides Physics.defaultSolverIterations. Must be positive.
    solverVelocityIterations	The solverVelocityIterations affects how how accurately Rigidbody joints and collision contacts are resolved. Overrides Physics.defaultSolverVelocityIterations. Must be positive.
    useGravity	Controls whether gravity affects this rigidbody.
    worldCenterOfMass	The center of mass of the rigidbody in world space (Read Only).
    */

    TransformComponent * mTransformComponent = nullptr;
    physx::PxRigidActor *mActor = nullptr;
    bool mIsKinematic = false;              // Controls whether physics affects the rigid-body.
};

