#include "AssetBaseMesh.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    MeshBase::MeshBase() = default;

    //-------------------------------------------------------------------------------------------------

    MeshBase::~MeshBase() = default;

    //-------------------------------------------------------------------------------------------------

    void MeshBase::initForWrite(
        uint32_t const vertexCount,
        uint32_t const indexCount,
        std::shared_ptr<SmartBlob> const & vertexBuffer,
        std::shared_ptr<SmartBlob> const & indexBuffer
    )
    {
        mVertexCount = vertexCount;
        mIndexCount = indexCount;
        MFA_ASSERT(vertexBuffer->memory.ptr != nullptr);
        MFA_ASSERT(vertexBuffer->memory.len > 0);
        mVertexData = vertexBuffer;
        MFA_ASSERT(indexBuffer->memory.ptr != nullptr);
        MFA_ASSERT(indexBuffer->memory.len > 0);
        mIndexData = indexBuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void MeshBase::finalizeData()
    {}

    //-------------------------------------------------------------------------------------------------

    uint32_t MeshBase::getVertexCount() const
    {
        return mVertexCount;
    }

    //-------------------------------------------------------------------------------------------------

    SmartBlob const * MeshBase::getVertexData() const
    {
        return mVertexData.get();
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t MeshBase::getIndexCount() const
    {
        return mIndexCount;
    }

    //-------------------------------------------------------------------------------------------------

    SmartBlob const * MeshBase::getIndexData() const
    {
        return mIndexData.get();
    }

    //-------------------------------------------------------------------------------------------------

    bool MeshBase::isValid() const
    {
        return mVertexCount > 0 &&
            mVertexData->memory.ptr != nullptr &&
            mVertexData->memory.len > 0 &&
            mIndexCount > 0 &&
            mIndexData->memory.ptr != nullptr &&
            mIndexData->memory.len > 0;
    }

    //-------------------------------------------------------------------------------------------------
}
