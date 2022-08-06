
#include "Physics.hpp"

#include "PhysicsTypes.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMatrix.hpp"

#include "physx/PxPhysicsAPI.h"

#include <thread>

// Based on snippet VehicleTank

using namespace physx;

namespace MFA::Physics
{

    static constexpr char const * DebugIP = "127.0.0.1";
    static constexpr int DebugPort = 5425;
    static constexpr int DebugTimeout = 1000;

    static constexpr float FixedDeltaTime = 1.0f / 30.0f;

    //-------------------------------------------------------------------------------------------------

    class SimulationEventCallback final : public PxSimulationEventCallback
    {
        void onAdvance(const PxRigidBody * const * bodyBuffer, const PxTransform * poseBuffer, const PxU32 count) override {}
        void onConstraintBreak(PxConstraintInfo * constraints, PxU32 count) override {}
        void onContact(const PxContactPairHeader & pairHeader, const PxContactPair * pairs, PxU32 nbPairs) override {}
        void onSleep(PxActor ** actors, PxU32 count) override {}
        void onTrigger(PxTriggerPair * pairs, PxU32 count) override {}
        void onWake(PxActor ** actors, PxU32 count) override {}
    };

    //-------------------------------------------------------------------------------------------------

    // TODO: Important: Research about PhysxWorldFilterShader
    PxFilterFlags PhysicsWorldFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags & pairFlags, const void * constantBlock, PxU32 constantBlockSize
    )
    {
        // Checking if layers should be ignored
        auto const layerMaskA = filterData0.word0;
        auto const layerA = filterData0.word1;

        auto const layerMaskB = filterData1.word0;
        auto const layerB = filterData1.word1;

        auto const aCollision = layerMaskA & layerB;
        auto const bCollision = layerMaskB & layerA;
        if (aCollision == 0 || bCollision == 0)
        {
            return PxFilterFlag::eSUPPRESS;
        }

        // all initial and persisting reports for everything, with per-point data
        pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        float stepResidualTime = 0.0f;
        PxDefaultAllocator allocator{};
        PxDefaultErrorCallback errorCallback{};
        SharedHandle<PxFoundation> foundation{};
        SharedHandle<PxPhysics> physics{};
        SharedHandle<PxDefaultCpuDispatcher> dispatcher{};
        SharedHandle<PxScene> scene{};
        SharedHandle<PxControllerManager> controllerManager{};
        SharedHandle<PxPvdTransport> transport{};
        SharedHandle<PxPvd> pvd{};
        SimulationEventCallback simulationEventCallback{};
        SharedHandle<PxMaterial> defaultMaterial{};
        SharedHandle<PxCooking> cooking{};
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    static void Step()
    {
        auto * scene = state->scene->Ptr();
        MFA_ASSERT(scene != nullptr);
        scene->simulate(FixedDeltaTime);
        scene->fetchResults(true);
    }

    //-------------------------------------------------------------------------------------------------

    void Init(InitParams const & params)
    {
        state = new State();

        auto const toleranceScale = PxTolerancesScale();

        state->foundation = CreateHandle(PxCreateFoundation(
            PX_PHYSICS_VERSION,
            state->allocator,
            state->errorCallback
        ));
        MFA_ASSERT(state->foundation != nullptr);

#ifdef MFA_DEBUG
        state->pvd = CreateHandle(PxCreatePvd(state->foundation->Ref()));
        state->transport = CreateHandle(PxDefaultPvdSocketTransportCreate(DebugIP, DebugPort, DebugTimeout));
        state->pvd->Ptr()->connect(state->transport->Ref(), PxPvdInstrumentationFlag::eALL);
#endif

        state->physics = CreateHandle(PxCreatePhysics(
            PX_PHYSICS_VERSION,
            state->foundation->Ref(),
            toleranceScale,
            true,
            state->pvd != nullptr ? state->pvd->Ptr() : nullptr
        ));
        MFA_ASSERT(state->physics != nullptr);

        // TODO: Do we need this many threads ?
        state->dispatcher = CreateHandle(PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency()));

        PxSceneDesc sceneDesc(state->physics->Ptr()->getTolerancesScale());
        sceneDesc.gravity = params.gravity;  // We can also manually control gravity for better control
        sceneDesc.cpuDispatcher = state->dispatcher->Ptr();
        sceneDesc.simulationEventCallback = &state->simulationEventCallback;
        sceneDesc.filterShader = PhysicsWorldFilterShader;

        state->scene = CreateHandle(state->physics->Ptr()->createScene(sceneDesc));

        state->controllerManager = CreateHandle(PxCreateControllerManager(state->scene->Ref()));

#ifdef MFA_DEBUG
        PxPvdSceneClient * pvdClient = state->scene->Ptr()->getScenePvdClient();
        if (pvdClient)
        {
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
#endif

        state->defaultMaterial = CreateMaterial(0.5f, 0.5f, 0.5f);

        state->cooking = CreateHandle(PxCreateCooking(
            PX_PHYSICS_VERSION,
            state->foundation->Ref(),
            PxCookingParams(toleranceScale))
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Update(float const deltaTime)
    {
        state->stepResidualTime -= deltaTime;
        while (state->stepResidualTime < 0.0f)
        {
            Step();
            state->stepResidualTime += FixedDeltaTime;
        }
    }

    //-------------------------------------------------------------------------------------------------

    // TODO: We need a system to check for memory leaks
    void Shutdown()
    {
        state->cooking = nullptr;
        state->defaultMaterial = nullptr;
        state->controllerManager = nullptr;
        state->scene = nullptr;
        state->dispatcher = nullptr;
        state->physics = nullptr;
        state->scene = nullptr;
        state->dispatcher = nullptr;
        state->physics = nullptr;
        state->pvd = nullptr;
        state->transport = nullptr;
        state->foundation = nullptr;
        MFA_ASSERT(state != nullptr);
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxRigidDynamic> CreateDynamicActor(PxTransform const & pxTransform)
    {
        return CreateHandle(state->physics->Ptr()->createRigidDynamic(pxTransform));
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxRigidStatic> CreateStaticActor(PxTransform const & pxTransform)
    {
        return CreateHandle(state->physics->Ptr()->createRigidStatic(pxTransform));
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxMaterial> CreateMaterial(
        float const staticFriction,
        float const dynamicFriction,
        float const restitution
    )
    {
        return CreateHandle(state->physics->Ptr()->createMaterial(
            staticFriction,
            dynamicFriction,
            restitution
        ));
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxMaterial> GetDefaultMaterial()
    {
        return state->defaultMaterial;
    }
    
    //-------------------------------------------------------------------------------------------------

    bool ValidateTriangleMesh(PxTriangleMeshDesc const & meshDesc)
    {
        return state->cooking->Ptr()->validateTriangleMesh(meshDesc);
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxTriangleMesh> CreateTriangleMesh(PxTriangleMeshDesc const & meshDesc)
    {
        SharedHandle<PxTriangleMesh> mesh = nullptr;

        PxDefaultMemoryOutputStream buf;
        PxTriangleMeshCookingResult::Enum result;

        bool const cookResult = state->cooking->Ptr()->cookTriangleMesh(meshDesc, buf, &result);
        MFA_ASSERT(cookResult == true);
        if (MFA_VERIFY(cookResult == true))
        {
            PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
            mesh = CreateHandle(state->physics->Ptr()->createTriangleMesh(input));
        }

        return mesh;
    }

    //-------------------------------------------------------------------------------------------------

    SharedHandle<PxShape> CreateShape(
        PxRigidActor & actor,
        PxGeometry const & geometry,
        PxMaterial const & material
    )
    {
        return CreateHandle(PxRigidActorExt::createExclusiveShape(
            actor,
            geometry,
            material
        ));
    }

    //-------------------------------------------------------------------------------------------------

    void AddActor(PxActor & actor)
    {
        state->scene->Ptr()->addActor(actor);
    }

    //-------------------------------------------------------------------------------------------------

    void RemoveActor(PxActor & actor)
    {
        state->scene->Ptr()->removeActor(actor);
    }

    //-------------------------------------------------------------------------------------------------

}

//#define PX_RELEASE(x)	if((x))	{ (x)->release(); (x) = nullptr; }

//PxFilterFlags PhysicsWorldFilterShader(    
//    PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
//    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
//    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
//{
//    /*pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
//    return PxFilterFlag::eDEFAULT;*/
//    // TODO Research about these flags
//    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS | PxPairFlag::ePRE_SOLVER_VELOCITY;
//    pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
//    pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
//    return PxFilterFlag::eDEFAULT;
//}
//
////-------------------------------------------------------------------------------------------------
//
//void PhysicsWorld::Init()
//{
//    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
//
//#ifndef NSO_NO_RENDERER
//    mPvd = PxCreatePvd(*mFoundation);	// Create a PhysX Visual Debugger instance.
//    PxPvdTransport *transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
//    mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
//#endif
//
//    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);
//    
//
//    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
//    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
//    // Dispatcher must use current thread only
//    // https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Manual/Threading.html
//    //  For platforms with a single execution core,
//    //  the CPU dispatcher can be created with zero worker threads (PxDefaultCpuDispatcherCreate(0)).
//    //  In this case all work will be executed on the thread that calls PxScene::simulate(),
//    //  which can be more efficient than using multiple threads.
//    mDispatcher = PxDefaultCpuDispatcherCreate(0);  
//    sceneDesc.cpuDispatcher = mDispatcher;
//    sceneDesc.simulationEventCallback = this;
//    sceneDesc.filterShader = PhysicsWorldFilterShader;
//    mScene = mPhysics->createScene(sceneDesc);
//
//    PxPvdSceneClient *pvdClient = mScene->getScenePvdClient();
//    if (pvdClient)
//    {
//        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
//        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
//        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
//    }
//
//    mControllerManager = PxCreateControllerManager(*mScene);
//}
//
////-------------------------------------------------------------------------------------------------
//
//void PhysicsWorld::Step()
//{
//    mScene->simulate(1.0f / 30.0f);
//    mScene->fetchResults(true);
//}
//
////-------------------------------------------------------------------------------------------------
//
//void PhysicsWorld::Cleanup()
//{
//    PX_RELEASE(mControllerManager);
//    PX_RELEASE(mScene);
//    PX_RELEASE(mDispatcher);
//    PX_RELEASE(mPhysics);
//
//    if (mPvd)
//    {
//        PxPvdTransport *transport = mPvd->getTransport();
//        mPvd->release();
//        mPvd = nullptr;
//        PX_RELEASE(transport);
//    }
//    PX_RELEASE(mFoundation);
//
//    printf("PhysicsWorld done.\n");
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::RayCast(
//    const Ray & ray,                        // [in] Ray origin
//    const float maxDistance,                // [in] Raycast max distance
//    RayCastHit * outResultBuffer,           // Result will be placed inside buffer
//    const uint8_t maxHitCount,              // Maximum hit count
//    const QueryFilterData & filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//
//    const PxQueryFilterData pxFilterData = filterData.GeneratePxQueryFilterData();
//    const PxHitFlags pxHitFlags = static_cast<PxHitFlag::Enum>(HitFlag::Default);
//
//    PxRaycastBuffer physxHitBuffer(
//        rayCastHitMemory,
//        NsoMath::Min<int>(PHYSX_QUERY_MAX_BUFFER_SIZE, maxHitCount)
//    ); // [out] Blocking and touching hits stored here
//
//    const bool foundAny = mScene->raycast(
//        MatrixUtils::GlmToPhysx(ray.origin),
//        MatrixUtils::GlmToPhysx(ray.direction),
//        maxDistance,
//        physxHitBuffer,
//        pxHitFlags,
//        pxFilterData
//    );
//    if (foundAny == false)
//    {
//        return 0;
//    }
//    // TODO MFA: Duplicate code with sphere cast, Try to make function
//    auto const hitCount = physxHitBuffer.getNbAnyHits();
//
//    if (physxHitBuffer.hasBlock)
//    {
//        const auto & blockingHit = physxHitBuffer.block;
//        outResultBuffer[0].AssignData(
//            blockingHit.actor,
//            MatrixUtils::PhysXToGlm(blockingHit.position),
//            blockingHit.distance,
//            true
//        );
//    }
//
//    const int blockOffset = physxHitBuffer.hasBlock ? 1 : 0;
//    for (uint32_t i = 0; i < physxHitBuffer.getNbTouches(); ++i)
//    {
//        const auto & physxHit = physxHitBuffer.getTouch(i);
//        auto & currentHit = outResultBuffer[i + blockOffset];
//        currentHit.AssignData(
//            physxHit.actor,
//            MatrixUtils::PhysXToGlm(physxHit.position),
//            physxHit.distance,
//            false
//        );
//    }
//    return hitCount;
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::geometryCast(
//    const Ray & ray,
//    const PxGeometry & geometry,
//    const glm::quat geometryOrientation,
//    const float maxDistance,
//    RayCastHit * outResultBuffer,
//    const uint8_t maxHitCount,
//    const QueryFilterData & filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//
//    const PxQueryFilterData pxFilterData = filterData.GeneratePxQueryFilterData();
//    const PxHitFlags pxHitFlags = static_cast<PxHitFlag::Enum>(HitFlag::Default);
//
//    PxSweepBuffer physxHitBuffer(
//        sweepMemory,
//        NsoMath::Min<int>(PHYSX_QUERY_MAX_BUFFER_SIZE, maxHitCount)
//    ); // [out] Blocking and touching hits stored here
//
//    const bool foundAny = mScene->sweep(
//        geometry,
//        MatrixUtils::PhysXTransform(ray.origin, geometryOrientation),
//        MatrixUtils::GlmToPhysx(ray.direction),
//        maxDistance,
//        physxHitBuffer,
//        pxHitFlags,
//        pxFilterData
//    );
//
//    if (foundAny == false)
//    {
//        return 0;
//    }
//
//    auto const hitCount = physxHitBuffer.getNbAnyHits();
//
//    if (physxHitBuffer.hasBlock)
//    {
//        const auto & blockingHit = physxHitBuffer.block;
//        outResultBuffer[0].AssignData(
//            blockingHit.actor,
//            MatrixUtils::PhysXToGlm(blockingHit.position),
//            blockingHit.distance,
//            true
//        );
//    }
//
//    const int blockOffset = physxHitBuffer.hasBlock ? 1 : 0;
//    for (uint32_t i = 0; i < physxHitBuffer.getNbTouches(); ++i)
//    {
//        const auto & physxHit = physxHitBuffer.getTouch(i);
//        auto & currentHit = outResultBuffer[i + blockOffset];
//        currentHit.AssignData(
//            physxHit.actor,
//            MatrixUtils::PhysXToGlm(physxHit.position),
//            physxHit.distance,
//            false
//        );
//    }
//    return hitCount;
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::SphereCast(
//    const Ray & ray,
//    const float radius,
//    const float maxDistance,
//    RayCastHit * outResultBuffer,
//    const uint8_t maxHitCount,
//    const QueryFilterData & filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//    // MFA: Computing sphere geometry for each sphere-cast is very bad. I think we should get geometry from shape-manager
//    const auto sphereGeometry = PxSphereGeometry(radius);
//    return geometryCast(
//        ray,
//        sphereGeometry,
//        MatrixUtils::QuaternionIdentity,
//        maxDistance,
//        outResultBuffer,
//        maxHitCount,
//        filterData
//    );
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::OverlapBox(
//    const glm::vec3 & center,
//    const glm::vec3 & halfExtents,
//    IEntity ** outResultBuffer,
//    const uint32_t maxHitCount,
//    const glm::quat & orientation,
//    const QueryFilterData &filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//
//    PxBoxGeometry geom;
//    geom.halfExtents = MatrixUtils::GlmToPhysx(halfExtents);
//
//    PxTransform pose = MatrixUtils::PhysXTransform(center, orientation);
//
//    return geometryOverlap(geom, pose, outResultBuffer, maxHitCount, filterData);
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::OverlapCapsule(
//    const glm::vec3 & point0,
//    const glm::vec3 & point1,
//    const float radius,
//    IEntity **outResultBuffer,
//    const uint32_t maxHitCount,
//    const QueryFilterData &filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//        
//    glm::vec3 center = (point0 + point1) / 2.0f;
//    glm::vec3 distance = point1 - point1;
//    glm::vec3 direction = glm::normalize(distance);
//    glm::quat rotation = MatrixUtils::GlmDirectionToRotation(direction, MatrixUtils::UpDirection);
//
//    PxTransform pose = MatrixUtils::PhysXTransform(center, rotation);
//
//    PxCapsuleGeometry geom;
//    geom.radius = radius;
//    geom.halfHeight = glm::length(distance) / 2.0f;
//
//    return geometryOverlap(geom, pose, outResultBuffer, maxHitCount, filterData);
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::OverlapSphere(
//    const glm::vec3 & position,
//    float radius,
//    IEntity **outResultBuffer,
//    const uint32_t maxHitCount,
//    const QueryFilterData &filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//
//    PxSphereGeometry geom;
//    geom.radius = radius;
//    
//    PxTransform pose = MatrixUtils::PhysXTransform(position);
//
//    return geometryOverlap(geom, pose, outResultBuffer, maxHitCount, filterData);
//}
//
////-------------------------------------------------------------------------------------------------
//
//int PhysicsWorld::geometryOverlap(
//    const PxGeometry &geometry,
//    const PxTransform &pose,
//    IEntity **outResultBuffer,
//    const uint32_t maxHitCount,
//    const QueryFilterData &filterData
//)
//{
//    NSO_ASSERT(outResultBuffer != nullptr);
//
//    PxOverlapBuffer overlapBuffer(mOverlapHits, NsoMath::Min<int>(PHYSX_QUERY_MAX_BUFFER_SIZE, maxHitCount));
//
//    mScene->overlap(geometry, pose, overlapBuffer, filterData.GeneratePxQueryFilterData());
//
//    PxU32 hitCount = overlapBuffer.getNbAnyHits();
//
//    for (PxU32 i = 0; i < hitCount; ++i)
//    {
//        const PxOverlapHit &hit = overlapBuffer.getAnyHit(i);
//        PxShape *shape = hit.shape;
//        PxRigidActor *actor = hit.actor;
//        if (!shape || !actor)
//            continue;
//
//        // Discarding Triggers
//        if (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
//            continue;
//
//        outResultBuffer[i] = (IEntity *)actor->userData;
//    }
//
//    return hitCount;
//}
//
////-------------------------------------------------------------------------------------------------
//
//void PhysicsWorld::onConstraintBreak(PxConstraintInfo *constraints, PxU32 count)
//{
//}
//
//void PhysicsWorld::onWake(PxActor **actors, PxU32 count)
//{
//    printf("OnAwake");
//}
//
//void PhysicsWorld::onSleep(PxActor **actors, PxU32 count)
//{
//}
//
//void PhysicsWorld::onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, const PxU32 nbPairs)
//{
//    for (uint32_t i = 0; i < nbPairs; i++)
//    {
//        const auto & currentPair = pairs[i];
//        
//        Collision collision {};
//        PxContactStreamIterator contactStreamIterator(
//            currentPair.contactPatches,
//            currentPair.contactPoints,
//            currentPair.getInternalFaceIndices(),
//            currentPair.patchCount,
//            currentPair.contactStreamSize
//        );
//
//        while(contactStreamIterator.hasNextPatch())
//        {
//            contactStreamIterator.nextPatch();
//            while(contactStreamIterator.hasNextContact())
//            {
//                contactStreamIterator.nextContact();
//                ContactPoint contactPoint {};
//                contactPoint.point = MatrixUtils::PhysXToGlm(contactStreamIterator.getContactPoint());
//                contactPoint.normal = MatrixUtils::PhysXToGlm(contactStreamIterator.getContactNormal());
//                /*collision.contactPoint = contactPoint;
//                contactPoint.penetration = contactStreamIterator.getSeparation();
//                contactPoint.normImpulse = pairs[i].flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES ? imp[0] : 0.0f;*/
//                collision.contactPoints.emplace_back(contactPoint);
//                
//            }
//        }
//
//        if (collision.contactPoints.empty())
//        {
//            return;
//        }
//
//        for (int j = 0; j < 2; j++)
//        {
//            const auto myIndex = j;
//            const auto otherIndex = (j + 1) % 2;
//
//            const auto * myShape = currentPair.shapes[myIndex];
//            NSO_ASSERT(myShape != nullptr);
//            auto * myCollider = static_cast<Collider *>(myShape->userData);
//            const auto * myActor = pairHeader.actors[myIndex];
//            NSO_ASSERT(myActor != nullptr);
//            auto * myEntity = static_cast<IEntity *>(myActor->userData);
//            
//            const auto * otherShape = currentPair.shapes[otherIndex];
//            NSO_ASSERT(otherShape != nullptr);
//            auto * otherCollider = static_cast<Collider *>(otherShape->userData);
//            const auto * otherActor = pairHeader.actors[otherIndex];
//            NSO_ASSERT(otherActor != nullptr);
//            auto * otherEntity = static_cast<IEntity *>(otherActor->userData);
//            
//            if (myEntity == nullptr || otherEntity == nullptr || otherCollider == nullptr || myCollider == nullptr)
//            {
//                break;
//            }
//
//            collision.otherEntity = otherEntity;
//            for (auto & contactPoint : collision.contactPoints)
//            {
//                contactPoint.thisCollider = myCollider;
//                contactPoint.otherCollider = otherCollider;
//            }
//            
//            EntityEvent event {};
//            event.type = EEntityEvent::PhysicsCollision;
//            event.nParam[0] = reinterpret_cast<intptr_t>(&collision);
//            myEntity->SendEvent(event);
//        }
//    }
//}
//
//void PhysicsWorld::onTrigger(PxTriggerPair *pairs, PxU32 count)
//{
//    printf("Trigger");
//}
//
//void PhysicsWorld::onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count)
//{
//}

//-------------------------------------------------------------------------------------------------
