#include "Asset_PBR_Mesh.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/job_system/JobSystem.hpp"

#include <foundation/PxVec3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA::AssetSystem::PBR
{

    //-------------------------------------------------------------------------------------------------

    std::vector<Primitive *> const & SubMesh::findPrimitives(AlphaMode alphaMode) const
    {
        switch (alphaMode)
        {
            case AlphaMode::Blend:
                return blendPrimitives;
            case AlphaMode::Mask:
                return maskPrimitives;
            case AlphaMode::Opaque:
                return opaquePrimitives;
            default:
                break;
        }
        MFA_LOG_ERROR("Alpha mode is not supported %d", static_cast<int>(alphaMode));
        return {};
    }

    //-------------------------------------------------------------------------------------------------

    bool MeshData::isValid() const
    {
        return rootNodes.empty() == false &&
            subMeshes.empty() == false &&
            nodes.empty() == false;
    }

    //-------------------------------------------------------------------------------------------------

    Mesh::Mesh() = default;

    Mesh::~Mesh() = default;

    //-------------------------------------------------------------------------------------------------

    void Mesh::initForWrite(
        uint32_t const vertexCount,
        uint32_t const indexCount,
        std::shared_ptr<SmartBlob> const & vertexBuffer,
        std::shared_ptr<SmartBlob> const & indexBuffer
    )
    {
        MeshBase::initForWrite(vertexCount, indexCount, vertexBuffer, indexBuffer);

        mData = std::make_shared<MeshData>();

        mIndicesStartingIndex = 0;
        mVerticesStartingIndex = 0;

        mNextVertexOffset = 0;
        mNextIndexOffset = 0;
    }

    //-------------------------------------------------------------------------------------------------

    // Calling this function is required to generate valid data
    void Mesh::finalizeData()
    {
        MeshBase::finalizeData();

        MFA_ASSERT(mNextIndexOffset == mIndexData->memory.len);
        MFA_ASSERT(mNextVertexOffset == mVertexData->memory.len);

        MFA_ASSERT(mData->nodes.empty() == false);
        MFA_ASSERT(mData->rootNodes.empty() == true);

        // Step one: Store parent index for each child
        for (int i = 0; i < static_cast<int>(mData->nodes.size()); ++i)
        {
            auto const & currentNode = mData->nodes[i];
            if (currentNode.children.empty() == false)
            {
                for (auto const child : currentNode.children)
                {
                    mData->nodes[child].parent = i;
                }
            }
        }
        // Step two: Cache parent nodes in a separate daa structure for faster access
        for (uint32_t i = 0; i < static_cast<uint32_t>(mData->nodes.size()); ++i)
        {
            auto const & currentNode = mData->nodes[i];
            if (currentNode.parent < 0)
            {
                mData->rootNodes.emplace_back(i);
            }
        }

        // Creating position min max for entire mesh based on subMeshes
        for (auto & subMesh : mData->subMeshes)
        {
            if (subMesh.hasPositionMinMax)
            {
                mData->hasPositionMinMax = true;
                // TODO: This logic is incorrect, For computing min and max correctly we should first do the skinning!
                // Position min
                if (subMesh.positionMin[0] < mData->positionMin[0])
                {
                    mData->positionMin[0] = subMesh.positionMin[0];
                }
                if (subMesh.positionMin[1] < mData->positionMin[1])
                {
                    mData->positionMin[1] = subMesh.positionMin[1];
                }
                if (subMesh.positionMin[2] < mData->positionMin[2])
                {
                    mData->positionMin[2] = subMesh.positionMin[2];
                }

                // Position max
                if (subMesh.positionMax[0] > mData->positionMax[0])
                {
                    mData->positionMax[0] = subMesh.positionMax[0];
                }
                if (subMesh.positionMax[1] > mData->positionMax[1])
                {
                    mData->positionMax[1] = subMesh.positionMax[1];
                }
                if (subMesh.positionMax[2] > mData->positionMax[2])
                {
                    mData->positionMax[2] = subMesh.positionMax[2];
                }
            }
            for (auto & primitive : subMesh.primitives)
            {
                switch (primitive.alphaMode)
                {
                    case AlphaMode::Opaque:
                        subMesh.opaquePrimitives.emplace_back(&primitive);
                    break;
                    case AlphaMode::Blend:
                        subMesh.blendPrimitives.emplace_back(&primitive);
                    break;
                    case AlphaMode::Mask:
                        subMesh.maskPrimitives.emplace_back(&primitive);
                    break;
                    default:
                    MFA_LOG_ERROR("Unhandled primitive alpha mode detected %d", static_cast<int>(primitive.alphaMode));
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t Mesh::insertSubMesh() const
    {
        mData->subMeshes.emplace_back();
        return static_cast<uint32_t>(mData->subMeshes.size() - 1);
    }

    //-------------------------------------------------------------------------------------------------

    void Mesh::insertPrimitive(
        uint32_t const subMeshIndex,
        Primitive & primitive,
        uint32_t const vertexCount,
        Vertex * vertices,
        uint32_t const indicesCount,
        Index * indices
    )
    {
        MFA_ASSERT(vertexCount > 0);
        MFA_ASSERT(indicesCount > 0);
        MFA_ASSERT(vertices != nullptr);
        MFA_ASSERT(indices != nullptr);
        primitive.vertexCount = vertexCount;
        primitive.indicesCount = indicesCount;
        primitive.indicesOffset = mNextIndexOffset;
        primitive.verticesOffset = mNextVertexOffset;
        primitive.indicesStartingIndex = mIndicesStartingIndex;
        primitive.verticesStartingIndex = mVerticesStartingIndex;
        uint32_t const verticesSize = sizeof(Vertex) * vertexCount;
        uint32_t const indicesSize = sizeof(Index) * indicesCount;
        MFA_ASSERT(mNextVertexOffset + verticesSize <= mVertexData->memory.len);
        MFA_ASSERT(mNextIndexOffset + indicesSize <= mIndexData->memory.len);
        ::memcpy(mVertexData->memory.ptr + mNextVertexOffset, vertices, verticesSize);
        ::memcpy(mIndexData->memory.ptr + mNextIndexOffset, indices, indicesSize);

        MFA_ASSERT(subMeshIndex < mData->subMeshes.size());
        auto & subMesh = mData->subMeshes[subMeshIndex];

        if (primitive.hasPositionMinMax)
        {
            subMesh.hasPositionMinMax = true;
            // Position min
            if (primitive.positionMin[0] < subMesh.positionMin[0])
            {
                subMesh.positionMin[0] = primitive.positionMin[0];
            }
            if (primitive.positionMin[1] < subMesh.positionMin[1])
            {
                subMesh.positionMin[1] = primitive.positionMin[1];
            }
            if (primitive.positionMin[2] < subMesh.positionMin[2])
            {
                subMesh.positionMin[2] = primitive.positionMin[2];
            }

            // Position max
            if (primitive.positionMax[0] > subMesh.positionMax[0])
            {
                subMesh.positionMax[0] = primitive.positionMax[0];
            }
            if (primitive.positionMax[1] > subMesh.positionMax[1])
            {
                subMesh.positionMax[1] = primitive.positionMax[1];
            }
            if (primitive.positionMax[2] > subMesh.positionMax[2])
            {
                subMesh.positionMax[2] = primitive.positionMax[2];
            }
        }
        subMesh.primitives.emplace_back(primitive);

        mNextVertexOffset += verticesSize;
        mNextIndexOffset += indicesSize;
        mIndicesStartingIndex += indicesCount;
        mVerticesStartingIndex += vertexCount;
    }

    //-------------------------------------------------------------------------------------------------

    void Mesh::insertNode(Node const & node)
    {
        mData->nodes.emplace_back(node);
    }

    //-------------------------------------------------------------------------------------------------

    void Mesh::insertSkin(Skin const & skin)
    {
        mData->skins.emplace_back(skin);
    }

    //-------------------------------------------------------------------------------------------------

    void Mesh::insertAnimation(Animation const & animation) const
    {
        mData->animations.emplace_back(animation);
    }

    //-------------------------------------------------------------------------------------------------

    bool Mesh::isValid() const
    {
        return mData->isValid() && MeshBase::isValid();
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<MeshData> const & Mesh::getMeshData() const
    {
        return mData;
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 Mesh::ComputeNodeLocalTransform(Node const & node) const
    {
        glm::mat4 matrix { 1 };
        matrix = glm::translate(matrix, Copy<glm::vec3>(node.translate));
        matrix = matrix * glm::toMat4(Copy<glm::quat>(node.rotation));
        matrix = glm::scale(matrix, Copy<glm::vec3>(node.scale));
        matrix = matrix * Copy<glm::mat4>(node.transform);
        return matrix;
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 Mesh::ComputeNodeGlobalTransform(Node const & node) const
    {
        auto matrix = ComputeNodeLocalTransform(node);

        auto * parentNode = &node;
        while (parentNode->HasParent())
        {
            parentNode = &mData->nodes[node.parent];
            matrix = ComputeNodeLocalTransform(*parentNode) * matrix;
        }

        return matrix;
    }

    //-------------------------------------------------------------------------------------------------

    void Mesh::PreparePhysicsPoints(PhysicsPointsCallback const & callback) const
    {
        std::vector<Physics::TriangleMeshDesc> triangleMeshes {};

        // TODO: Important, Apply the matrix for each of them
        // TODO: Multi-thread

        for (auto const & node : mData->nodes)
        {
            if (node.hasSubMesh())
            {
                auto matrix = ComputeNodeGlobalTransform(node);
                auto const & subMesh = mData->subMeshes[node.subMeshIndex];
                for (auto const & primitive : subMesh.primitives)
                {
                    triangleMeshes.emplace_back();
                    auto & triangleMesh = triangleMeshes.back();
                    MFA_ASSERT(primitive.indicesCount % 3 == 0);

                    triangleMesh.trianglesCount = primitive.indicesCount / 3;
                    triangleMesh.trianglesStride = 3 * sizeof(Index);
                    triangleMesh.triangleBuffer = Memory::Alloc(primitive.indicesCount * sizeof(Index));

                    triangleMesh.pointsStride = sizeof(physx::PxVec3);
                    triangleMesh.pointsCount = primitive.vertexCount;
                    triangleMesh.pointsBuffer = Memory::Alloc(primitive.vertexCount * sizeof(physx::PxVec3));

                    auto * pointsArray = triangleMesh.pointsBuffer->memory.as<physx::PxVec3>();

                    auto * trianglesArray = triangleMesh.triangleBuffer->memory.as<Index>();

                    auto const * vertexArray = reinterpret_cast<Vertex *>(mVertexData->memory.ptr + primitive.verticesOffset);
                    auto const * indicesArray = reinterpret_cast<Index *>(mIndexData->memory.ptr + primitive.indicesOffset);

                    for (uint32_t i = 0; i < primitive.vertexCount; ++i)
                    {
                        auto const & vertex = vertexArray[i];
                        static_assert(sizeof(vertex.position) == sizeof(pointsArray[i]));

                        glm::vec4 vertex4 { vertex.position[0], vertex.position[1], vertex.position[2], 1.0f };
                        vertex4 = matrix * vertex4;

                        Copy(pointsArray[i], vertex4);
                    }

                    for (uint32_t i = 0; i < primitive.indicesCount; ++i)
                    {
                        trianglesArray[i] = indicesArray[i] - primitive.verticesStartingIndex;
                    }
                }
            }
        }

        callback(triangleMeshes);
    }

    //-------------------------------------------------------------------------------------------------

}
