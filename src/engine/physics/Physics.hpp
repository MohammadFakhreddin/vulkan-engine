#pragma once

#include <PxActor.h>

#include "PhysicsTypes.hpp"

#include <foundation/PxVec3.h>

namespace physx
{
    class PxTriangleMeshDesc;
    class PxConvexMesh;
    class PxMaterial;
    class PxGeometry;
    class PxRigidActor;
    class PxShape;
    class PxTransform;
    class PxRigidDynamic;
    class PxRigidStatic;
    class PxActor;
}

namespace MFA::Physics// final : public IPhysicsWorld, public PxSimulationEventCallback
{
    struct InitParams
    {
        physx::PxVec3 gravity {};
    };
    void Init(InitParams const & params);

    void Update(float deltaTime);

    void Shutdown();

    SharedHandle<physx::PxRigidDynamic> CreateDynamicActor(physx::PxTransform const & pxTransform);

    SharedHandle<physx::PxRigidStatic> CreateStaticActor(physx::PxTransform const & pxTransform);

    // Actor releases the exclusive shapes
    physx::PxShape * CreateShape(
        physx::PxRigidActor & actor,
        physx::PxGeometry const & geometry,
        physx::PxMaterial const & material
    );

    SharedHandle<physx::PxMaterial> CreateMaterial(
        float staticFriction,
        float dynamicFriction,
        float restitution
    );

    SharedHandle<physx::PxMaterial> GetDefaultMaterial();

    bool ValidateTriangleMesh(physx::PxTriangleMeshDesc const & meshDesc);

    SharedHandle<physx::PxTriangleMesh> CreateTriangleMesh(physx::PxTriangleMeshDesc const & meshDesc);

    void AddActor(physx::PxActor & actor);

    void RemoveActor(physx::PxActor & actor);

    //void Init() override;
    //void Step() override;
    //void Cleanup() override;

    //[[nodiscard]]
    //PxPhysics * GetPhysX() override { return mPhysics; }

    //[[nodiscard]]
    //PxScene * GetScene() override { return mScene; }

    //[[nodiscard]]
    //physx::PxControllerManager * GetCCTManager() override { return mControllerManager; }

    /**
     * Raycast sources are the following:
     * 1. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/SceneQueries.html
     * 2. https://gameworksdocs.nvidia.com/PhysX/4.0/documentation/PhysXGuide/Manual/SceneQueries.html
     * 3. https://gameworksdocs.nvidia.com/PhysX/4.0/documentation/PhysXGuide/Manual/SceneQueries.html#pxfilterdata-fixed-function-filtering
     */

    /***
     * 
     * \return Returns number of hitCounts
     */
    //[[nodiscard]]
    //int RayCast(
    //    const Ray & ray,                        // [in] Ray origin
    //    float maxDistance,                      // [in] Raycast max distance
    //    RayCastHit * outResultBuffer,           // Result will be placed inside buffer
    //    uint8_t maxHitCount,                    // Maximum hit count
    //    const QueryFilterData & filterData
    //) override;

    //[[nodiscard]]
    //int SphereCast(
    //    const Ray & ray,                        // [in] Ray origin
    //    float radius,                           // [in] Sphere radius
    //    float maxDistance,                      // [in] Raycast max distance
    //    RayCastHit * outResultBuffer,
    //    uint8_t maxHitCount,
    //    const QueryFilterData & filterData
    //) override;

    //[[nodiscard]]
    //int OverlapBox(
    //    const glm::vec3 & center,
    //    const glm::vec3 & halfExtents,
    //    IEntity **outResultBuffer,
    //    const uint32_t maxHitCount,
    //    const glm::quat & orientation,
    //    const QueryFilterData &filterData
    //) override;

    //[[nodiscard]]
    //int OverlapCapsule(
    //    const glm::vec3 & point0,
    //    const glm::vec3 & point1,
    //    float radius,
    //    IEntity **outResultBuffer,
    //    const uint32_t maxHitCount,
    //    const QueryFilterData &filterData
    //) override;

    //[[nodiscard]]
    //int OverlapSphere(
    //    const glm::vec3 & position,
    //    const float radius,
    //    IEntity **outResultBuffer,
    //    const uint32_t maxHitCount,
    //    const QueryFilterData &filterData
    //) override;

//private:
//
//    [[nodiscard]]
//    int geometryCast(
//        const Ray & ray,                        // [in] Ray origin
//        const PxGeometry & geometry,            // [in] Geometry shape // Ex: Sphere
//        glm::quat geometryOrientation,
//        float maxDistance,                      // [in] Raycast max distance
//        RayCastHit * outResultBuffer,
//        uint8_t maxHitCount,
//        const QueryFilterData & filterData
//    );
//
//    [[nodiscard]]
//    int geometryOverlap(
//        const PxGeometry &geometry,            // [in] Geometry shape // Ex: Sphere
//        const PxTransform &pose,
//        IEntity **outResultBuffer,
//        const uint32_t maxHitCount,
//        const QueryFilterData &filterData
//    );
//
//    // Inherited via PxSimulationEventCallback
//    void onConstraintBreak(PxConstraintInfo * constraints, PxU32 count) override;
//    void onWake(PxActor ** actors, PxU32 count) override;
//    void onSleep(PxActor ** actors, PxU32 count) override;
//    void onContact(const PxContactPairHeader & pairHeader, const PxContactPair * pairs, PxU32 nbPairs) override;
//    void onTrigger(PxTriggerPair * pairs, PxU32 count) override;
//    void onAdvance(const PxRigidBody * const * bodyBuffer, const PxTransform * poseBuffer, const PxU32 count) override;
//
//    PxDefaultAllocator mAllocator;
//    PxDefaultErrorCallback mErrorCallback;
//
//    PxFoundation * mFoundation = nullptr;
//    PxPhysics * mPhysics = nullptr;
//
//    PxDefaultCpuDispatcher * mDispatcher = nullptr;
//    PxScene * mScene = nullptr;
//
//    PxPvd * mPvd = nullptr;
//    PxControllerManager *mControllerManager = nullptr;
//
//    static constexpr PxU32 PHYSX_QUERY_MAX_BUFFER_SIZE = 256;
//    PxRaycastHit rayCastHitMemory[PHYSX_QUERY_MAX_BUFFER_SIZE];
//    PxSweepHit sweepMemory[PHYSX_QUERY_MAX_BUFFER_SIZE];
//    PxOverlapHit mOverlapHits[PHYSX_QUERY_MAX_BUFFER_SIZE];
    
};
