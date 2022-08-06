#pragma once

#include "LayerMask.h"
#include "EntitySystem/IEntityComponent.h"
#include "Collider.h"
#include "EntitySystem/TransformComponent.h"

namespace physx
{
    class PxRigidActor;
    class PxShape;
    class PxMaterial;
    class PxController;
    class PxObstacleContext;
    struct PxFilterData;
    class PxQueryFilterCallback;
    class PxControllerFilterCallback;
}

class TransformComponent;

enum class CollisionFlags_t
{
    None = 0,
    Sides = (1 << 0),   // Character is colliding to the sides.
    Above = (1 << 1),   // Character has collision above.
    Below = (1 << 2),   // Character has collision below.
};
using CollisionFlags = CEnumFlags<CollisionFlags_t>;

class CharacterControllerComponent final : public Collider
{
public:

    [[nodiscard]]
    EntityEventMask GetEventMask() const override
    {
        return EEntityEvent::Initialize | EEntityEvent::PhysicsCollision | EEntityEvent::Update | EEntityEvent::Remove;
    }

    // The current relative velocity of the Character (see notes).
    [[nodiscard]]
    glm::vec3 GetVelocity() const;

    [[nodiscard]]
    float GetRadius() const noexcept
    {
        return mRadius;
    }

    void SetRadius(const float radius)
    {
        mRadius = radius;
    }

    [[nodiscard]]
    float GetHeight() const noexcept
    {
        return mHeight;
    }

    void SetHeight(const float height)
    {
        mHeight = height;
    }

    [[nodiscard]]
    glm::vec3 GetCenter() const noexcept
    {
        return mCenter;
    }

    void SetCenter(const glm::vec3 & center)
    {
        mCenter = center;
    }

    [[nodiscard]]
    float GetSlopeLimit() const noexcept
    {
        return mSlopeLimit;
    }

    void SetSlopeLimit(const float slopeLimit)
    {
        mSlopeLimit = slopeLimit;
    }

    [[nodiscard]]
    float GetStepOffset() const noexcept
    {
        return mStepOffset;
    }

    void SetStepOffset(const float stepOffset)
    {
        mStepOffset = stepOffset;
    }

    [[nodiscard]]
    float GetSkinWidth() const noexcept
    {
        return mSkinWidth;
    }

    void SetSkinWidth(const float skinWidth)
    {
        mSkinWidth = skinWidth;
    }

    [[nodiscard]]
    float GetMinMoveDistance()
    {
        return mMinMoveDistance;
    }

    void SetMinMoveDistance(float minMoveDistance)
    {
        mMinMoveDistance = minMoveDistance;
    }

    CollisionFlags Move(const glm::vec3 &motion, float deltaTime);

    [[nodiscard]]
    physx::PxRigidActor *GetActor() const noexcept;

    // TODO Replace it with setActive inside component class
    void EnableController(bool enable);

private:

    bool init() override;

    void shutdown() override;

    void onTransformPositionChanged();

    void createController();
    void destroyController();

    LayerMask mLayerMask {};

    float mRadius {};                   // The radius of the character's capsule.
    float mHeight {};                   // The height of the character's capsule.
    glm::vec3 mCenter {};               // The center of the character's capsule relative to the transform's position.
    float mSlopeLimit = 40;             // The character controllers slope limit in degrees.
    float mStepOffset = 0.5f;           // The character controllers step offset in meters.
    float mSkinWidth = 0.08f;           // The character's collision skin width.
    float mMinMoveDistance = 0.001f;      // The minimum move distance of the character controller.
    float mInvisibleWallHeight = 0;
    float mMaxJumpHeight = 0;

    physx::PxController *mController = nullptr;
    TransformComponent *mTransform = nullptr;

    physx::PxMaterial *mMaterial = nullptr;
    physx::PxObstacleContext *mObstacleContext = nullptr;	            // User-defined additional obstacles
    const physx::PxFilterData *mFilterData = nullptr;		            // User-defined filter data for 'move' function
    physx::PxQueryFilterCallback *mFilterCallback = nullptr;	        // User-defined filter data for 'move' function
    physx::PxControllerFilterCallback *mCCTFilterCallback = nullptr;	// User-defined filter data for 'move' function

    glm::vec3 mTargetMotion {};

    int mPositionListenerId = -1;
    bool mIsPositionChangedByController = false;

    bool mIsEnabled = true;
};
