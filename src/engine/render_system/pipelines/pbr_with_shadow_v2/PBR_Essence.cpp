#include "PBR_Essence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/BedrockMemory.hpp"

#include <utility>

#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

//-------------------------------------------------------------------------------------------------

using namespace MFA::AS::PBR;

// We need other overrides for easier use as well
MFA::PBR_Essence::PBR_Essence(
    std::shared_ptr<RT::GpuModel> gpuModel,
    std::shared_ptr<AS::PBR::MeshData> meshData
)
    : EssenceBase(gpuModel->nameId)
    , mGpuModel(std::move(gpuModel))
    , mMeshData(std::move(meshData))
{
    MFA_ASSERT(mGpuModel != nullptr);
    
    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (auto const & subMesh : mMeshData->subMeshes) {
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;

            mPrimitivesBuffer = RF::CreateLocalUniformBuffer(bufferSize, 1);
            auto const stageBuffer = RF::CreateStageBuffer(bufferSize, 1);
            
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

            auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();
            RF::UpdateLocalBuffer(
                commandBuffer,
                *mPrimitivesBuffer->buffers[0],
                *stageBuffer->buffers[0],
                primitiveData->memory
            );
            RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);

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

MFA::RT::BufferGroup const * MFA::PBR_Essence::getPrimitivesBuffer() const noexcept {
    return mPrimitivesBuffer.get();
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

MFA::RenderTypes::GpuModel const * MFA::PBR_Essence::getGpuModel() const
{
    return mGpuModel.get();
}

//-------------------------------------------------------------------------------------------------

MeshData const * MFA::PBR_Essence::getMeshData() const
{
    return mMeshData.get();
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::createGraphicDescriptorSet(
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    RT::GpuTexture const & errorTexture
)
{
    auto const & textures = mGpuModel->textures;

    mGraphicDescriptorSet = RF::CreateDescriptorSets(
        descriptorPool,
        RF::GetMaxFramesPerFlight(),
        descriptorSetLayout
    );
    
    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex)
    {
        auto const & descriptorSet = mGraphicDescriptorSet.descriptorSets[frameIndex];
        MFA_VK_VALID_ASSERT(descriptorSet);

        DescriptorSetSchema descriptorSetSchema{ descriptorSet };

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////

        // Primitives
        VkDescriptorBufferInfo primitiveBufferInfo{
            .buffer = mPrimitivesBuffer->buffers[0]->buffer,
            .offset = 0,
            .range = mPrimitivesBuffer->bufferSize,
        };
        descriptorSetSchema.AddUniformBuffer(&primitiveBufferInfo);

        // TODO Each one need their own sampler
        // Textures
        MFA_ASSERT(textures.size() <= 64);
        // We need to keep imageInfos alive
        std::vector<VkDescriptorImageInfo> imageInfos{};
        for (auto const & texture : textures)
        {
            imageInfos.emplace_back(VkDescriptorImageInfo{
                .sampler = nullptr,
                .imageView = texture->imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
        for (auto i = static_cast<uint32_t>(textures.size()); i < 64; ++i)
        {
            imageInfos.emplace_back(VkDescriptorImageInfo{
                .sampler = nullptr,
                .imageView = errorTexture.imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
        MFA_ASSERT(imageInfos.size() == 64);
        descriptorSetSchema.AddImage(
            imageInfos.data(),
            static_cast<uint32_t>(imageInfos.size())
        );

        descriptorSetSchema.UpdateDescriptorSets();
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindForGraphicPipeline(RT::CommandRecordState const & recordState) const
{
    bindGraphicDescriptorSet(recordState);
    bindVertexBuffer(recordState);
    bindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindVertexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindVertexBuffer(recordState, *mGpuModel->meshBuffers->vertexBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indexBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const
{
    RF::AutoBindDescriptorSet(
        recordState,
        RF::UpdateFrequency::PerEssence,
        mGraphicDescriptorSet
    );
}

//-------------------------------------------------------------------------------------------------
