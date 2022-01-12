#include "AssetBaseMesh.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    MeshBase::MeshBase(uint32_t const vertexBufferCount_)
        : requiredVertexBufferCount(vertexBufferCount_)
    {}

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
        mVertexBuffer = vertexBuffer;
        MFA_ASSERT(indexBuffer->memory.ptr != nullptr);
        MFA_ASSERT(indexBuffer->memory.len > 0);
        mIndexBuffer = indexBuffer;
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

    SmartBlob const * MeshBase::getVertexBuffer() const
    {
        return mVertexBuffer.get();
    }

    //-------------------------------------------------------------------------------------------------

    uint32_t MeshBase::getIndexCount() const
    {
        return mIndexCount;
    }

    //-------------------------------------------------------------------------------------------------

    SmartBlob const * MeshBase::getIndexBuffer() const
    {
        return mIndexBuffer.get();
    }

    //-------------------------------------------------------------------------------------------------

    bool MeshBase::isValid() const
    {
        return mVertexCount > 0 &&
            mVertexBuffer->memory.ptr != nullptr &&
            mVertexBuffer->memory.len > 0 &&
            mIndexCount > 0 &&
            mIndexBuffer->memory.ptr != nullptr &&
            mIndexBuffer->memory.len > 0;
    }

    //-------------------------------------------------------------------------------------------------
}
