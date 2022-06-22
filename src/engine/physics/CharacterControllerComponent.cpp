#include "CharacterControllerComponent.h"

#include "EntitySystem/TransformComponent.h"
#include "Common/EntitySystem/IEntity.h"

#include "characterkinematic/PxCapsuleController.h"
#include "characterkinematic/PxControllerManager.h"

#include "Common/System/ISystem.h"
#include "Common/Physics/IPhysicsWorld.h"

#include "Utils/MatrixUtils.h"
#include "Common/Math/NsoMath.h"

#include "PxPhysicsAPI.h"

using namespace physx;

//-------------------------------------------------------------------------------------------------

bool CharacterControllerComponent::init()
{
    auto * transform = m_pEntity->GetComponent<TransformComponent>();
    if (!NSO_VERIFY(transform != nullptr))
        return false;

    mTransform = transform;
    
    auto * physicsWorld = gEnv->pPhysicsWorld;

    // TODO We can create mMaterial once for all characterController
    mMaterial = physicsWorld->GetPhysX()->createMaterial(0.5f, 0.5f, 0.1f);
    if (!NSO_VERIFY(mMaterial != nullptr))
    {
        printf("fatalError createMaterial failed!");
        return false;
    }

    createController();

    mPositionListenerId = mTransform->PositionChangeSignal.Register(
        this,
        &CharacterControllerComponent::onTransformPositionChanged
    );

    return false;
}

//-------------------------------------------------------------------------------------------------

void CharacterControllerComponent::shutdown()
{
    Collider::shutdown();

    mTransform->PositionChangeSignal.UnRegister(mPositionListenerId);
}

//-------------------------------------------------------------------------------------------------

void CharacterControllerComponent::onTransformPositionChanged()
{
    if (mIsEnabled == false)
    {
        return;
    }

    if (mIsPositionChangedByController == true)
    {
        mIsPositionChangedByController = false;
        return;
    }

    auto const newPosition = mTransform->GetPosition();
    mController->setPosition(PxExtendedVec3{
        static_cast<PxExtended>(newPosition.x),
        static_cast<PxExtended>(newPosition.y),
        static_cast<PxExtended>(newPosition.z)
    });
}

//-------------------------------------------------------------------------------------------------

CollisionFlags CharacterControllerComponent::Move(const glm::vec3 & motion, float deltaTime)
{
    if (mIsEnabled == false)
    {
        return CollisionFlags_t::None;
    }

    mTargetMotion = motion;

    const PxVec3 disp = MatrixUtils::GlmToPhysx(mTargetMotion);
    const PxReal dtime = deltaTime;
    const PxControllerFilters filters(mFilterData, mFilterCallback, mCCTFilterCallback);
    const PxU32 flags = mController->move(disp, mMinMoveDistance, dtime, filters, mObstacleContext);

    glm::vec3 position = MatrixUtils::PhysXToGlm(mController->getPosition());
    //printf("CCT Move: %f %f %f\n", position.x, position.y, position.z);
    mIsPositionChangedByController = true;
    mTransform->SetPosition(position);

    return CollisionFlags(CollisionFlags_t(flags));
}

//-------------------------------------------------------------------------------------------------

physx::PxRigidActor * CharacterControllerComponent::GetActor() const noexcept
{
    PxRigidActor * actor = nullptr;
    if (mController != nullptr)
    {
        actor = mController->getActor();
    }
    return actor;
}

//-------------------------------------------------------------------------------------------------

void CharacterControllerComponent::EnableController(const bool enable)
{
    if (mIsEnabled == enable)
    {
        return;
    }
    mIsEnabled = enable;
    if (mIsEnabled)
    {
        auto const position = mTransform->GetPosition();
        mController->setPosition(PxExtendedVec3{
            static_cast<PxExtended>(position.x),
            static_cast<PxExtended>(position.y),
            static_cast<PxExtended>(position.z)
        });
    } else
    {
        mController->setPosition(PxExtendedVec3 {
            -1000.0,
            -1000.0,
            -1000.0
        });
    }
}

//-------------------------------------------------------------------------------------------------

glm::vec3 CharacterControllerComponent::GetVelocity() const
{
    return MatrixUtils::PhysXToGlm(mController->getActor()->getLinearVelocity());
}

//-------------------------------------------------------------------------------------------------

void CharacterControllerComponent::createController()
{
    NSO_ASSERT(mController == nullptr);

    PxControllerManager * cctManager = gEnv->pPhysicsWorld->GetCCTManager();
    NSO_ASSERT(cctManager != nullptr);

    const glm::vec3 position = mTransform->GetPosition();

    PxCapsuleControllerDesc capsuleDesc;
    capsuleDesc.height = mHeight - 2 * mRadius;
    capsuleDesc.radius = mRadius;
    capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
    //capsuleDesc.density = mProxyDensity;
    //capsuleDesc.scaleCoeff = mProxyScale;
    capsuleDesc.material = mMaterial;
    capsuleDesc.position = PxExtendedVec3(position.x, position.y, position.z);
    capsuleDesc.slopeLimit = ::cosf(NsoMath::Deg2Rad(mSlopeLimit));
    capsuleDesc.contactOffset = mSkinWidth;
    capsuleDesc.stepOffset = mStepOffset;
    capsuleDesc.invisibleWallHeight = mInvisibleWallHeight;
    capsuleDesc.maxJumpHeight = mMaxJumpHeight;
    // capsuleDesc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
    capsuleDesc.reportCallback = nullptr; // mReportCallback;
    capsuleDesc.behaviorCallback = nullptr; //mBehaviorCallback;
    //capsuleDesc.volumeGrowth = mVolumeGrowth;


    PxController * ctrl = cctManager->createController(capsuleDesc);
    NSO_ASSERT(ctrl);

    mController = ctrl;

    auto * actor = mController->getActor();
    NSO_ASSERT(actor != nullptr);
    actor->userData = m_pEntity;

    mController->getActor()->getShapes(&mShape, 1);
    NSO_ASSERT(mShape != nullptr);
}

//-------------------------------------------------------------------------------------------------

void CharacterControllerComponent::destroyController()
{
    NSO_ASSERT(mController != nullptr);
    mController->release();
    mController = nullptr;
}

//-------------------------------------------------------------------------------------------------
