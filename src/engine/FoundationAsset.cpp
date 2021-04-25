#include "FoundationAsset.hpp"

namespace MFA::AssetSystem {

uint8_t Texture::ComputeMipCount(Dimensions const & dimensions) {
    uint32_t const max_dimension = Math::Max(
        dimensions.width, 
        Math::Max(
            dimensions.height, 
            static_cast<uint32_t>(dimensions.depth)
        )
    );
    for (uint8_t i = 0; i < 32; ++i)
        if ((1ULL << i) >= max_dimension)
            return 1 + i;
    return 33;
}

size_t Texture::MipSizeBytes (
    Format format,
    uint16_t const slices,
    Dimensions const & mipLevelDimension
)
{
    auto const & d = mipLevelDimension;
    size_t const p = FormatTable[static_cast<unsigned>(format)].bits_total / 8;
    return p * slices * d.width * d.height * d.depth;
}

Texture::Dimensions Texture::MipDimensions (
    uint8_t const mipLevel,
    uint8_t const mipCount,
    Dimensions const originalImageDims
)
{
    Dimensions ret = {};
    if (mipLevel < mipCount) {
        uint32_t const pow = mipLevel;
        uint32_t const add = (1 << pow) - 1;
        ret.width = (originalImageDims.width + add) >> pow;
        ret.height = (originalImageDims.height + add) >> pow;
        ret.depth = static_cast<uint16_t>((originalImageDims.depth + add) >> pow);
    }
    return ret;
}

size_t Texture::CalculateUncompressedTextureRequiredDataSize(
    Format const format,
    uint16_t const slices,
    Dimensions const & dims,
    uint8_t const mipCount
) {
    size_t ret = 0;
    for (uint8_t mip_level = 0; mip_level < mipCount; mip_level++) {
        ret += MipSizeBytes(
            format, 
            slices, 
            MipDimensions(mip_level, mipCount, dims)
        );
    }
    return ret;
}

void Texture::initForWrite(
    const Format format,
    const uint16_t slices,
    const uint16_t depth,
    SamplerConfig const * sampler,
    Blob const & buffer
) {
    MFA_ASSERT(mBuffer.ptr == nullptr);
    MFA_ASSERT(format != Format::INVALID);
    mFormat = format;
    MFA_ASSERT(slices > 0);
    mSlices = slices;
    MFA_ASSERT(depth > 0);
    mDepth = depth;
    if (sampler != nullptr && sampler->isValid) {
        mSampler = *sampler;
    }
    mBuffer = buffer;
}

void Texture::addMipmap(
    Dimensions const & dimension,
    CBlob const & data
) {
    MFA_ASSERT(data.ptr != nullptr);
    MFA_ASSERT(data.len > 0);

    MFA_ASSERT(mPreviousMipWidth == -1 || mPreviousMipWidth > static_cast<int>(dimension.width));
    MFA_ASSERT(mPreviousMipHeight == -1 || mPreviousMipHeight > static_cast<int>(dimension.height));
    mPreviousMipWidth = dimension.width;
    mPreviousMipHeight = dimension.height;

    uint32_t dataLen = static_cast<uint32_t>(data.len);
    mMipmapInfos.emplace_back(MipmapInfo {
        .offset = mCurrentOffset,
        .size = dataLen,
        .dimension = dimension,
    });
    uint64_t nextOffset = mCurrentOffset + dataLen;
    MFA_ASSERT(mBuffer.ptr != nullptr);
    MFA_ASSERT(nextOffset <= mBuffer.len);
    ::memcpy(mBuffer.ptr + mCurrentOffset, data.ptr, data.len);
    mCurrentOffset = nextOffset;

    ++mMipCount;
}

size_t Texture::mipOffsetInBytes(uint8_t const mip_level, uint8_t const slice_index) const {
    size_t ret = 0;
    if(mip_level < mMipCount && slice_index < mSlices) {
        ret = mMipmapInfos[mip_level].offset + slice_index * mMipmapInfos[mip_level].size;
    }
    return ret;
}

bool Texture::isValid() const {
    return
        mFormat != Format::INVALID && 
        mSlices > 0 && 
        mMipCount > 0 && 
        mMipmapInfos.empty() == false && 
        mBuffer.ptr != nullptr && 
        mBuffer.len > 0;
}

void Mesh::initForWrite(
    const uint32_t vertexCount,
    const uint32_t indexCount,
    const Blob & vertexBuffer,
    const Blob & indexBuffer
) {
    MFA_ASSERT(mVertexBuffer.ptr == nullptr);
    MFA_ASSERT(mIndexBuffer.ptr == nullptr);
    mVertexCount = vertexCount;
    mNextVertexOffset = 0;
    mIndexCount = indexCount;
    mNextIndexOffset = 0;
    MFA_ASSERT(vertexBuffer.ptr != nullptr);
    MFA_ASSERT(vertexBuffer.len > 0);
    mVertexBuffer = vertexBuffer;
    MFA_ASSERT(indexBuffer.ptr != nullptr);
    MFA_ASSERT(indexBuffer.len > 0);
    mIndexBuffer = indexBuffer;
}

void Mesh::finalizeData() {
    MFA_ASSERT(mNextIndexOffset == mIndexBuffer.len);
    MFA_ASSERT(mNextVertexOffset == mVertexBuffer.len);
    for (int i = 0; i < static_cast<int>(mNodes.size()); ++i) {
        auto const & currentNode = mNodes[i];
        if (currentNode.children.empty() == false) {
            for (auto const child : currentNode.children) {
                mNodes[child].parent = i;
            }
        }
    }
}

uint32_t Mesh::insertSubMesh() {
    mSubMeshes.emplace_back();
    return static_cast<uint32_t>(mSubMeshes.size() - 1);
}

void Mesh::insertPrimitive(
    uint32_t subMeshIndex,
    Primitive && primitive, 
    uint32_t const vertexCount, 
    Vertex * vertices, 
    uint32_t const indicesCount, 
    Index * indices
) {
    MFA_ASSERT(vertexCount > 0);
    MFA_ASSERT(indicesCount > 0);
    MFA_ASSERT(vertices != nullptr);
    MFA_ASSERT(indices != nullptr);
    primitive.vertexCount = vertexCount;
    primitive.indicesCount = indicesCount;
    primitive.indicesOffset = mNextIndexOffset;
    primitive.verticesOffset = mNextVertexOffset;
    primitive.indicesStartingIndex = mNextStartingIndex;
    uint32_t const verticesSize = sizeof(Vertex) * vertexCount;
    uint32_t const indicesSize = sizeof(Index) * indicesCount;
    MFA_ASSERT(mNextVertexOffset + verticesSize <= mVertexBuffer.len);
    MFA_ASSERT(mNextIndexOffset + indicesSize <= mIndexBuffer.len);
    ::memcpy(mVertexBuffer.ptr + mNextVertexOffset, vertices, verticesSize);
    ::memcpy(mIndexBuffer.ptr + mNextIndexOffset, indices, indicesSize);
    MFA_ASSERT(subMeshIndex < mSubMeshes.size());
    mSubMeshes[subMeshIndex].primitives.emplace_back(primitive);
    mNextVertexOffset += verticesSize;
    mNextIndexOffset += indicesSize;
    mNextStartingIndex += indicesCount;
}

void Mesh::insertNode(Node const & node) {
    mNodes.emplace_back(node);
}

bool Mesh::isValid() const {
    return
        mSubMeshes.empty() == false && 
        mNodes.empty() == false &&
        mVertexCount > 0 &&
        mVertexBuffer.ptr != nullptr &&
        mVertexBuffer.len > 0 &&
        mIndexCount > 0 &&
        mIndexBuffer.ptr != nullptr &&
        mIndexBuffer.len > 0;
}

void Mesh::revokeBuffers(Blob & outVertexBuffer, Blob & outIndexBuffer) {
    outVertexBuffer = mVertexBuffer;
    outIndexBuffer = mIndexBuffer;
    mVertexBuffer = {};
    mIndexBuffer = {};
}


}
