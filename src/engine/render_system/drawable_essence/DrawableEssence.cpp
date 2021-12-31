#include "DrawableEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

// We need other overrides for easier use as well
MFA::DrawableEssence::DrawableEssence(
    std::shared_ptr<RT::GpuModel> const & gpuModel,
    std::shared_ptr<AS::Mesh> const & cpuMesh
)
    : Essence(gpuModel)
{
    MFA_ASSERT(mGpuModel != nullptr);

    mCpuMesh = cpuMesh;
    MFA_ASSERT(mCpuMesh != nullptr);

    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (uint32_t i = 0; i < mCpuMesh->GetSubMeshCount(); ++i) {
            auto const & subMesh = mCpuMesh->GetSubMeshByIndex(i);
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);

            auto const primitiveData = Memory::Alloc(bufferSize);
            
            auto * primitivesArray = primitiveData->memory.as<PrimitiveInfo>();
            for (uint32_t i = 0; i < mCpuMesh->GetSubMeshCount(); ++i) {
                auto const & subMesh = mCpuMesh->GetSubMeshByIndex(i);
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
        auto const animationCount = mCpuMesh->GetAnimationsCount();
        for (uint32_t i = 0; i < animationCount; ++i) {
            auto const & animation = mCpuMesh->GetAnimationByIndex(i);
            MFA_ASSERT(mAnimationNameLookupTable.find(animation.name) == mAnimationNameLookupTable.end());
            mAnimationNameLookupTable[animation.name] = static_cast<int>(i);
        }
    }
}

//-------------------------------------------------------------------------------------------------

MFA::DrawableEssence::~DrawableEssence() = default;

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup const & MFA::DrawableEssence::getPrimitivesBuffer() const noexcept {
    return *mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

uint32_t MFA::DrawableEssence::getPrimitiveCount() const noexcept
{
    return mPrimitiveCount;
}

//-------------------------------------------------------------------------------------------------

int MFA::DrawableEssence::getAnimationIndex(char const * name) const noexcept {
    auto const findResult = mAnimationNameLookupTable.find(name);
    if (findResult != mAnimationNameLookupTable.end()) {
        return findResult->second;
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------

MFA::AssetSystem::Mesh const * MFA::DrawableEssence::getCpuMesh() const
{
    return mCpuMesh.get();
}

//-------------------------------------------------------------------------------------------------
