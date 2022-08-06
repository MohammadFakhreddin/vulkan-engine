#pragma once

#include "IEntity.h"
#include "Math/NsoMath.h"
#include "Collider.h"
#include "EntitySystem/TransformComponent.h"

#include <PxQueryFiltering.h>
#include <PxRigidActor.h>

using namespace physx;

enum class FilterFlag
{
    Static				= (1<<0),	//!< Traverse static shapes

    Dynamic			    = (1<<1),	//!< Traverse dynamic shapes

    PreFilter			= (1<<2),	//!< Run the pre-intersection-test filter (see #PxQueryFilterCallback::preFilter())

    PostFilter			= (1<<3),	//!< Run the post-intersection-test filter (see #PxQueryFilterCallback::postFilter())

    AnyHit			    = (1<<4),	//!< Abort traversal as soon as any hit is found and return it via callback.block.
                                    //!< Helps query performance. Both eTOUCH and eBLOCK hitTypes are considered hits with this flag.

    NoBlock			    = (1<<5),	//!< All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
                                    //!< This is also an optimization hint that may improve query performance.

    Reserved			= (1<<15),	//!< Reserved for internal use

};

using FilterFlags = CEnumFlags<FilterFlag>;

enum class HitFlag
{
    Position					= (1<<0),	//!< "position" member of #PxQueryHit is valid
    Normal						= (1<<1),	//!< "normal" member of #PxQueryHit is valid
    UV							= (1<<3),	//!< "u" and "v" barycentric coordinates of #PxQueryHit are valid. Not applicable to sweep queries.
    AssumeNoInitialOverlap	    = (1<<4),	//!< Performance hint flag for sweeps when it is known upfront there's no initial overlap.
                                            //!< NOTE: using this flag may cause undefined results if shapes are initially overlapping.
    MeshMultiple				= (1<<5),	//!< Report all hits for meshes rather than just the first. Not applicable to sweep queries.
    MeshAny					    = (1<<6),	//!< Report any first hit for meshes. If neither eMESH_MULTIPLE nor eMESH_ANY is specified,
                                            //!< a single closest hit will be reported for meshes.
    MeshBothSides			    = (1<<7),	//!< Report hits with back faces of mesh triangles. Also report hits for raycast
                                            //!< originating on mesh surface and facing away from the surface normal. Not applicable to sweep queries.
                                            //!< Please refer to the user guide for heightfield-specific differences.
    PreciseSweep				= (1<<8),	//!< Use more accurate but slower narrow phase sweep tests.
                                            //!< May provide better compatibility with PhysX 3.2 sweep behavior.
    MTD						    = (1<<9),	//!< Report the minimum translation depth, normal and contact point.
    FaceIndex					= (1<<10),	//!< "face index" member of #PxQueryHit is valid

    Default					    = Position, // Originally it was ePOSITION|eNORMAL|eFACE_INDEX,

    /** \brief Only this subset of flags can be modified by pre-filter. Other modifications will be discarded. */
    eMODIFIABLE_FLAGS			= MeshMultiple|MeshBothSides|AssumeNoInitialOverlap|PreciseSweep
};
typedef CEnumFlags<HitFlag> HitFlags;

struct QueryFilterData
{
    // TODO: Read about flags
    FilterFlags filterFlags {FilterFlag::Dynamic, FilterFlag::Static};

    LayerMask layerMask0 {};
    LayerMask layerMask1 {};
    LayerMask layerMask2 {};
    LayerMask layerMask3 {};

    [[nodiscard]]
    PxQueryFilterData GeneratePxQueryFilterData() const
    {
        PxQueryFilterData filterData;
        filterData.flags = static_cast<PxQueryFlag::Enum>(filterFlags.Value());
        filterData.data.word0 = layerMask0.GetValue();
        filterData.data.word1 = layerMask1.GetValue();
        filterData.data.word2 = layerMask2.GetValue();
        filterData.data.word3 = layerMask3.GetValue();
        return filterData;
    }
};

struct Ray
{

    Ray() = default;

    // Creates a ray starting at origin along direction.
    explicit Ray(const glm::vec3 & origin_, const glm::vec3 & direction_)
        : origin(origin_)
        , direction(glm::normalize(direction_))
    {}

    // The origin point of the ray.
    glm::vec3 origin {};

    // The direction of the ray.
    glm::vec3 direction {};

    [[nodiscard]]
    glm::vec3 GetPointFromZeroOrigin(const float distance) const
    {
        NSO_ASSERT(distance >= 0);
        return direction * distance;
    }

    [[nodiscard]]
    glm::vec3 GetPointFromRayOrigin(const float distance) const
    {
        NSO_ASSERT(distance >= 0);
        return direction * distance + origin;
    }
    
};

struct RayCastHit
{
    bool isBlocking = false;

    PxRigidActor * actor {};

    // The GameObject whose collider you are colliding with. (Read Only).
    IEntity * entity {};

    // The impact point in world space where the ray hit the collider.
    glm::vec3 position {};
    
    // The distance from the ray's origin to the impact point.
    float distance {};

    void AssignData(
        PxRigidActor * actor_,
        const glm::vec3 & position_,
        const float distance_,
        const bool isBlocking_
    ) {
        NSO_ASSERT(actor_ != nullptr);
        actor = actor_;
        position = position_;
        distance = distance_;
        NSO_ASSERT(actor->userData != nullptr);
        entity = static_cast<IEntity *>(actor->userData);
        
        isBlocking = isBlocking_;
    }

};
