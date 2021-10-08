#include "FoundationAsset.hpp"

#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"

namespace MFA::AssetSystem {

//-------------------------------------------------------------------------------------------------

int Base::serialize(CBlob const & writeBuffer) {
    MFA_CRASH("Not implemented");
}
void Base::deserialize(CBlob const & readBuffer) {
    MFA_CRASH("Not implemented");
}
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

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------

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

    auto const dataLen = static_cast<uint32_t>(data.len);
    {
        MipmapInfo mipmapInfo {};
        mipmapInfo.offset = mCurrentOffset;
        mipmapInfo.size = dataLen;
        mipmapInfo.dimension = dimension;
        mMipmapInfos.emplace_back(mipmapInfo);
    }
    uint64_t nextOffset = mCurrentOffset + dataLen;
    MFA_ASSERT(mBuffer.ptr != nullptr);
    MFA_ASSERT(nextOffset <= mBuffer.len);
    ::memcpy(mBuffer.ptr + mCurrentOffset, data.ptr, data.len);
    mCurrentOffset = nextOffset;

    ++mMipCount;
}

//-------------------------------------------------------------------------------------------------

size_t Texture::mipOffsetInBytes(uint8_t const mip_level, uint8_t const slice_index) const {
    size_t ret = 0;
    if(mip_level < mMipCount && slice_index < mSlices) {
        ret = mMipmapInfos[mip_level].offset + slice_index * mMipmapInfos[mip_level].size;
    }
    return ret;
}

//-------------------------------------------------------------------------------------------------

bool Texture::isValid() const {
    return
        mFormat != Format::INVALID && 
        mSlices > 0 && 
        mMipCount > 0 && 
        mMipmapInfos.empty() == false && 
        mBuffer.ptr != nullptr && 
        mBuffer.len > 0;
}

//-------------------------------------------------------------------------------------------------

Blob Texture::revokeBuffer() {
    auto const buffer = mBuffer;
    mBuffer = {};
    return buffer;
}

//-------------------------------------------------------------------------------------------------

CBlob Texture::GetBuffer() const noexcept {
    return mBuffer;
}

//-------------------------------------------------------------------------------------------------

Texture::Format Texture::GetFormat() const noexcept {
    return mFormat;
}

//-------------------------------------------------------------------------------------------------

uint16_t Texture::GetSlices() const noexcept {
    return mSlices;
}

//-------------------------------------------------------------------------------------------------

uint8_t Texture::GetMipCount() const noexcept {
    return mMipCount;
}

//-------------------------------------------------------------------------------------------------

Texture::MipmapInfo const & Texture::GetMipmap(uint8_t const mipLevel) const noexcept {
    return mMipmapInfos[mipLevel];
}

//-------------------------------------------------------------------------------------------------

Texture::MipmapInfo const * Texture::GetMipmaps() const noexcept {
    return mMipmapInfos.data();
}

//-------------------------------------------------------------------------------------------------

void Texture::SetSampler(SamplerConfig * sampler) {
    MFA_ASSERT(sampler != nullptr);
    mSampler = *sampler;
}

//-------------------------------------------------------------------------------------------------

SamplerConfig const & Texture::GetSampler() const noexcept {
    return mSampler;
}

//-------------------------------------------------------------------------------------------------

void Mesh::InitForWrite(
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

//-------------------------------------------------------------------------------------------------

// Calling this function is required to generate valid data
void Mesh::FinalizeData() {
    MFA_ASSERT(mNextIndexOffset == mIndexBuffer.len);
    MFA_ASSERT(mNextVertexOffset == mVertexBuffer.len);
    MFA_ASSERT(mNodes.empty() == false);
    MFA_ASSERT(mRootNodes.empty() == true);
   
    // Pass one: Store parent index for each child
    for (int i = 0; i < static_cast<int>(mNodes.size()); ++i) {
        auto const & currentNode = mNodes[i];
        if (currentNode.children.empty() == false) {
            for (auto const child : currentNode.children) {
                mNodes[child].parent = i;
            }
        }
    }
    // Pass tow: Cache parent nodes in a separate daa structure for faster access
    for (uint32_t i = 0; i < static_cast<uint32_t>(mNodes.size()); ++i) {
        auto const & currentNode = mNodes[i];
        if (currentNode.parent < 0) {
            mRootNodes.emplace_back(i);
        }
    }

    // Creating position min max for entire mesh based on subMeshes
    for (auto const & subMesh : mSubMeshes)
    {
        if (subMesh.hasPositionMinMax)
        {
            mHasPositionMinMax = true;

            // Position min
            if (subMesh.positionMin[0] < mPositionMin[0])
            {
                mPositionMin[0] = subMesh.positionMin[0];
            }
            if (subMesh.positionMin[1] < mPositionMin[1])
            {
                mPositionMin[1] = subMesh.positionMin[1];
            }
            if (subMesh.positionMin[2] < mPositionMin[2])
            {
                mPositionMin[2] = subMesh.positionMin[2];
            }

            // Position max
            if (subMesh.positionMax[0] > mPositionMax[0])
            {
                mPositionMax[0] = subMesh.positionMax[0];
            }
            if (subMesh.positionMax[1] > mPositionMax[1])
            {
                mPositionMax[1] = subMesh.positionMax[1];
            }
            if (subMesh.positionMax[2] > mPositionMax[2])
            {
                mPositionMax[2] = subMesh.positionMax[2];
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

uint32_t Mesh::InsertSubMesh() {
    mSubMeshes.emplace_back();
    return static_cast<uint32_t>(mSubMeshes.size() - 1);
}

//-------------------------------------------------------------------------------------------------

void Mesh::InsertPrimitive(
    uint32_t const subMeshIndex,
    Primitive & primitive, 
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
    auto & subMesh = mSubMeshes[subMeshIndex];
    
    if (primitive.hasPositionMinMax) {
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
    mNextStartingIndex += indicesCount;
}

//-------------------------------------------------------------------------------------------------

void Mesh::InsertNode(Node const & node) {
    mNodes.emplace_back(node);
}

//-------------------------------------------------------------------------------------------------

void Mesh::InsertSkin(Skin const & skin) {
    mSkins.emplace_back(skin);
}

//-------------------------------------------------------------------------------------------------

void Mesh::InsertAnimation(Animation const & animation) {
    mAnimations.emplace_back(animation);
}

//-------------------------------------------------------------------------------------------------

bool Mesh::IsValid() const {
    return
        mRootNodes.empty() == false &&
        mSubMeshes.empty() == false && 
        mNodes.empty() == false &&
        mVertexCount > 0 &&
        mVertexBuffer.ptr != nullptr &&
        mVertexBuffer.len > 0 &&
        mIndexCount > 0 &&
        mIndexBuffer.ptr != nullptr &&
        mIndexBuffer.len > 0;
}

//-------------------------------------------------------------------------------------------------

CBlob Mesh::GetVerticesBuffer() const noexcept {
    return mVertexBuffer;
}

//-------------------------------------------------------------------------------------------------

CBlob Mesh::GetIndicesBuffer() const noexcept {
    return mIndexBuffer;
}

//-------------------------------------------------------------------------------------------------

uint32_t Mesh::GetSubMeshCount() const noexcept {
    return static_cast<uint32_t>(mSubMeshes.size());
}

//-------------------------------------------------------------------------------------------------

SubMesh const & Mesh::GetSubMeshByIndex(uint32_t const index) const noexcept {
    MFA_ASSERT(index < mSubMeshes.size());
    return mSubMeshes[index];
}

//-------------------------------------------------------------------------------------------------

SubMesh & Mesh::GetSubMeshByIndex(uint32_t const index) {
    MFA_ASSERT(index < mSubMeshes.size());
    return mSubMeshes[index];
}

//-------------------------------------------------------------------------------------------------

Mesh::Node const & Mesh::GetNodeByIndex(uint32_t const index) const noexcept {
    MFA_ASSERT(index >= 0);
    MFA_ASSERT(index < mNodes.size());
    return mNodes[index];
}

//-------------------------------------------------------------------------------------------------

Mesh::Node & Mesh::GetNodeByIndex(uint32_t const index) {
    MFA_ASSERT(index >= 0);
    MFA_ASSERT(index < mNodes.size());
    return mNodes[index];
}

//-------------------------------------------------------------------------------------------------

Mesh::Skin const & Mesh::GetSkinByIndex(uint32_t const index) const noexcept {
    MFA_ASSERT(index >= 0);
    MFA_ASSERT(index < mSkins.size());
    return mSkins[index];
}

//-------------------------------------------------------------------------------------------------

Mesh::Animation const & Mesh::GetAnimationByIndex(uint32_t const index) const noexcept {
    MFA_ASSERT(index >= 0);
    MFA_ASSERT(index < mAnimations.size());
    return mAnimations[index];
}

//-------------------------------------------------------------------------------------------------

uint32_t Mesh::GetIndexOfRootNode(uint32_t const index) const {
    MFA_ASSERT(index < mRootNodes.size());
    return mRootNodes[index];
}

//-------------------------------------------------------------------------------------------------

Mesh::Node & Mesh::GetRootNodeByIndex(uint32_t const index) {
    MFA_ASSERT(index < mRootNodes.size());
    auto const rootNodeIndex = mRootNodes[index];
    MFA_ASSERT(rootNodeIndex < mNodes.size());
    return mNodes[rootNodeIndex];
}

//-------------------------------------------------------------------------------------------------

Mesh::Node const & Mesh::GetRootNodeByIndex(uint32_t const index) const {
    MFA_ASSERT(index < mRootNodes.size());
    auto const rootNodeIndex = mRootNodes[index];
    MFA_ASSERT(rootNodeIndex < mNodes.size());
    return mNodes[rootNodeIndex];
}

//-------------------------------------------------------------------------------------------------

void Mesh::RevokeBuffers(Blob & outVertexBuffer, Blob & outIndexBuffer) {
    outVertexBuffer = mVertexBuffer;
    outIndexBuffer = mIndexBuffer;
    mVertexBuffer = {};
    mIndexBuffer = {};
}

//-------------------------------------------------------------------------------------------------


}
