#include "FoundationAsset.hpp"

namespace MFA::AssetSystem {

U8 Texture::ComputeMipCount(Dimensions const & dimensions) {
    U32 const max_dimension = Math::Max(
        dimensions.width, 
        Math::Max(
            dimensions.height, 
            static_cast<uint32_t>(dimensions.depth)
        )
    );
    for (U8 i = 0; i < 32; ++i)
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
    uint8_t const mip_level,
    uint8_t const mip_count,
    Dimensions const original_image_dims
)
{
    Dimensions ret = {};
    if (mip_level < mip_count) {
        uint32_t const pow = mip_count - 1 - mip_level;
        uint32_t const add = (1 << pow) - 1;
        ret.width = (original_image_dims.width + add) >> pow;
        ret.height = (original_image_dims.height + add) >> pow;
        ret.depth = static_cast<uint16_t>((original_image_dims.depth + add) >> pow);
    }
    return ret;
}

size_t Texture::CalculateUncompressedTextureRequiredDataSize(
    Format const format,
    U16 const slices,
    Dimensions const & dims,
    U8 const mipCount
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
    const U16 slices,
    const U8 mipCount,
    const U16 depth,
    Sampler const * sampler,
    Blob const & buffer
) {
    MFA_ASSERT(mBuffer.ptr == nullptr);
    MFA_ASSERT(format != Format::INVALID);
    mFormat = format;
    MFA_ASSERT(slices > 0);
    mSlices = slices;
    MFA_ASSERT(mipCount > 0);
    mMipCount = mipCount;
    MFA_ASSERT(depth > 0);
    mDepth = depth;
    if (sampler != nullptr) {
        MFA_ASSERT(sampler->isValid);
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
    mMipmapInfos.emplace_back(MipmapInfo {
        .offset = mCurrentOffset,
        .size = static_cast<U32>(data.len),
        .dimension = dimension,
    });
    U32 nextOffset = mCurrentOffset + static_cast<U32>(data.len);
    MFA_ASSERT(mBuffer.ptr != nullptr);
    MFA_ASSERT(nextOffset <= mBuffer.len);
    ::memcpy(mBuffer.ptr + mCurrentOffset, data.ptr, data.len);
    mCurrentOffset = nextOffset;
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
    const U32 vertexCount,
    const U32 indexCount,
    const Blob & vertexBuffer,
    const Blob & indexBuffer
) {
    MFA_ASSERT(mVertexBuffer.ptr == nullptr);
    MFA_ASSERT(mIndexBuffer.ptr == nullptr);
    mVertexCount = vertexCount;
    mCurrentVertexOffset = 0;
    mIndexCount = indexCount;
    mCurrentIndexOffset = 0;
    MFA_ASSERT(vertexBuffer.ptr != nullptr);
    MFA_ASSERT(vertexBuffer.len > 0);
    mVertexBuffer = vertexBuffer;
    MFA_ASSERT(indexBuffer.ptr != nullptr);
    MFA_ASSERT(indexBuffer.len > 0);
    mIndexBuffer = indexBuffer;
}

U32 Mesh::insertSubMesh() {
    mSubMeshes.emplace_back();
    return static_cast<U32>(mSubMeshes.size() - 1);
}

void Mesh::insertPrimitive(
    U32 subMeshIndex,
    Primitive && primitive, 
    U32 const vertexCount, 
    Vertex * vertices, 
    U32 const indicesCount, 
    Index * indices
) {
    primitive.vertexCount = vertexCount;
    primitive.indicesCount = indicesCount;
    primitive.verticesOffset = mCurrentVertexOffset;
    primitive.indicesStartingIndex = mCurrentStartingIndex;
    U32 const verticesSize = sizeof(Vertex) * vertexCount;
    U32 const indicesSize = sizeof(Index) * indicesCount;
    MFA_ASSERT(mCurrentVertexOffset + verticesSize <= mVertexBuffer.len);
    MFA_ASSERT(mCurrentIndexOffset + indicesSize <= mIndexBuffer.len);
    ::memcpy(mVertexBuffer.ptr, vertices, verticesSize);
    ::memcpy(mIndexBuffer.ptr, indices, indicesSize);
    MFA_ASSERT(subMeshIndex < mSubMeshes.size());
    mSubMeshes[subMeshIndex].primitives.emplace_back(primitive);
    mCurrentVertexOffset += verticesSize;
    mCurrentIndexOffset += indicesSize;
    mCurrentStartingIndex += indicesCount;
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
