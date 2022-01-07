#include "PBR_Essence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/BedrockMemory.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

using namespace MFA::AS::PBR;

// We need other overrides for easier use as well
MFA::PBR_Essence::PBR_Essence(
    std::shared_ptr<RT::GpuModel> const & gpuModel,
    std::shared_ptr<MeshData> const & meshData
)
    : EssenceBase(gpuModel)
    , mMeshData(meshData)
{
    MFA_ASSERT(mGpuModel != nullptr);
    
    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (auto const & subMesh : mMeshData->subMeshes) {
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);

            auto const primitiveData = Memory::Alloc(bufferSize);
            
            auto * primitivesArray = primitiveData->memory.as<PrimitiveInfo>();
            for (auto const & subMesh : mMeshData->subMeshes) {
                for (auto const & primitive : subMesh.primitives) {
                    // Copy primitive into primitive info
                    PrimitiveInfo & primitiveInfo = primitivesArray[primitive.uniqueId];
                    primitiveInfo.baseColorTextureIndex = primitive.hasBaseColorTexture ? primitive.baseColorTextureIndex : -1;
                    primitiveInfo.metallicFactor = primitive.metallicFactor;
                    primitiveInfo.roughnessFactor = primitive.roughnessFactor;
                    primitiveInfo.metallicRoughnessTextureIndex = primitive.hasMetallicRoughnessTexture ? primitive.metallicRoughnessTextureIndex : -1;
                    primitiveInfo.normalTextureIndex = primitive.hasNormalTexture ? primitive.normalTextureIndex : -1;
                    primitiveInfo.emissiveTextureIndex = primitive.hasEmissiveTexture ? primitive.emissiveTextureIndex : -1;
                    primitiveInfo.hasSkin = primitive.hasSkin ? 1 : 0;
                    primitiveInfo.occlusionTextureIndex = primitive.hasOcclusionTexture ? primitive.occlusionTextureIndex : -1;

                    ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                    static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                    static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));

                    primitiveInfo.alphaMode = static_cast<int>(primitive.alphaMode);
                    primitiveInfo.alphaCutoff = primitive.alphaCutoff;
                }
            }

            RF::UpdateUniformBuffer(*mPrimitivesBuffer->buffers[0], primitiveData->memory);
        }
    }
    {// Animations
        int animationIndex = 0;
        for (auto const & animation : mMeshData->animations) {
            MFA_ASSERT(mAnimationNameLookupTable.find(animation.name) == mAnimationNameLookupTable.end());
            mAnimationNameLookupTable[animation.name] = animationIndex;
            ++animationIndex;
        }
    }
}

//-------------------------------------------------------------------------------------------------

MFA::PBR_Essence::~PBR_Essence() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup const & MFA::PBR_Essence::getPrimitivesBuffer() const noexcept {
    return *mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

uint32_t MFA::PBR_Essence::getPrimitiveCount() const noexcept
{
    return mPrimitiveCount;
}

//-------------------------------------------------------------------------------------------------

int MFA::PBR_Essence::getAnimationIndex(char const * name) const noexcept {
    auto const findResult = mAnimationNameLookupTable.find(name);
    if (findResult != mAnimationNameLookupTable.end()) {
        return findResult->second;
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------

MeshData const * MFA::PBR_Essence::getMeshData() const
{
    return mMeshData.get();
}

//-------------------------------------------------------------------------------------------------
