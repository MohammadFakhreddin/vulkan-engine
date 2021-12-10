#include "DrawableEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"

#include <utility>

//-------------------------------------------------------------------------------------------------

// We need other overrides for easier use as well
MFA::DrawableEssence::DrawableEssence(std::shared_ptr<RT::GpuModel> gpuModel)
    : mGpuModel(std::move(gpuModel))
{
    MFA_ASSERT(mGpuModel != nullptr);

    auto const & mesh = mGpuModel->model->mesh;
    MFA_ASSERT(mesh != nullptr);

    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (uint32_t i = 0; i < mesh->GetSubMeshCount(); ++i) {
            auto const & subMesh = mesh->GetSubMeshByIndex(i);
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);

            auto const primitiveData = Memory::Alloc(bufferSize);
            
            auto * primitivesArray = primitiveData->memory.as<PrimitiveInfo>();
            for (uint32_t i = 0; i < mesh->GetSubMeshCount(); ++i) {
                auto const & subMesh = mesh->GetSubMeshByIndex(i);
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

            RF::UpdateUniformBuffer(*mPrimitivesBuffer->buffers[0], primitiveData->memory);
        }
    }
    {// Animations
        auto const animationCount = mesh->GetAnimationsCount();
        for (uint32_t i = 0; i < animationCount; ++i) {
            auto const & animation = mesh->GetAnimationByIndex(i);
            MFA_ASSERT(mAnimationNameLookupTable.find(animation.name) == mAnimationNameLookupTable.end());
            mAnimationNameLookupTable[animation.name] = static_cast<int>(i);
        }
    }
}

//-------------------------------------------------------------------------------------------------

MFA::DrawableEssence::~DrawableEssence() = default;

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::GpuModelId MFA::DrawableEssence::GetId() const
{
    return mGpuModel->id;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel * MFA::DrawableEssence::GetGpuModel() const {
    return mGpuModel.get();
}

//-------------------------------------------------------------------------------------------------

MFA::RT::UniformBufferGroup const & MFA::DrawableEssence::GetPrimitivesBuffer() const noexcept {
    return *mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

uint32_t MFA::DrawableEssence::GetPrimitiveCount() const noexcept
{
    return mPrimitiveCount;
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
    const VkDescriptorPool descriptorPool,
    uint32_t const descriptorSetCount,
    const VkDescriptorSetLayout descriptorSetLayout
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

void MFA::DrawableEssence::BindVertexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindVertexBuffer(recordState, *mGpuModel->meshBuffers->verticesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::DrawableEssence::BindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indicesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::DrawableEssence::BindDescriptorSetGroup(RT::CommandRecordState const & recordState) const
{
    RF::BindDescriptorSet(
        recordState,
        RenderFrontend::DescriptorSetType::PerEssence,
        mDescriptorSetGroup
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::DrawableEssence::BindAllRenderRequiredData(RT::CommandRecordState const & recordState) const
{
    BindDescriptorSetGroup(recordState);
    BindVertexBuffer(recordState);
    BindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------
