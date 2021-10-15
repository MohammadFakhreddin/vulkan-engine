#include "DrawableEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"

//-------------------------------------------------------------------------------------------------

// We need other overrides for easier use as well
MFA::DrawableEssence::DrawableEssence(char const * name, RT::GpuModel const & model_)
    : mName(name)
    , mGpuModel(model_)
{
    MFA_ASSERT(mName.empty() == false);
    MFA_ASSERT(mGpuModel.valid);
    MFA_ASSERT(mGpuModel.model.mesh.IsValid());
    
    auto & mesh = mGpuModel.model.mesh;

    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
            auto const & subMesh = mesh.GetSubMeshByIndex(i);
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);
            Blob primitiveData = Memory::Alloc(bufferSize);
            MFA_DEFER {
                Memory::Free(primitiveData);
            };

            auto * primitivesArray = primitiveData.as<PrimitiveInfo>();
            for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
                auto const & subMesh = mesh.GetSubMeshByIndex(i);
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
                    // TODO Occlusion

                    ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                    static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                    static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));
                }
            }

            RF::UpdateUniformBuffer(mPrimitivesBuffer.buffers[0], primitiveData);
        }
    }
    {// Animations
        auto const animationCount = mesh.GetAnimationsCount();
        for (uint32_t i = 0; i < animationCount; ++i) {
            auto const & animation = mesh.GetAnimationByIndex(i);
            MFA_ASSERT(mAnimationNameLookupTable.find(animation.name) == mAnimationNameLookupTable.end());
            mAnimationNameLookupTable[animation.name] = static_cast<int>(i);
        }
    }
}

//-------------------------------------------------------------------------------------------------

MFA::DrawableEssence::~DrawableEssence() {
    if (mPrimitivesBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mPrimitivesBuffer);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel const & MFA::DrawableEssence::GetGpuModel() const {
    return mGpuModel;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup const & MFA::DrawableEssence::GetPrimitivesBuffer() const noexcept {
    return mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

std::string const & MFA::DrawableEssence::GetName() const noexcept {
    return mName;
}

//-------------------------------------------------------------------------------------------------

int MFA::DrawableEssence::GetAnimationIndex(char const * name) const noexcept {
    auto const findResult = mAnimationNameLookupTable.find(name);
    if (findResult != mAnimationNameLookupTable.end()) {
        return findResult->second;
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------

  
MFA::RT::DescriptorSetGroup const & MFA::DrawableEssence::CreateDescriptorSetGroup(
    VkDescriptorPool descriptorPool,
    uint32_t const descriptorSetCount,
    VkDescriptorSetLayout descriptorSetLayout
)
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == false);
    mDescriptorSetGroup = RF::CreateDescriptorSets(
        descriptorPool,
        descriptorSetCount,
        descriptorSetLayout
    );
    mIsDescriptorSetGroupValid = true;
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DescriptorSetGroup const & MFA::DrawableEssence::GetDescriptorSetGroup() const
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------
