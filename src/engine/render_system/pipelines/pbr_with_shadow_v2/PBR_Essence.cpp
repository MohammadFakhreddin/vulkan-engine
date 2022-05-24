#include "PBR_Essence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"

#include <utility>

#include "engine/BedrockMemory.hpp"

//-------------------------------------------------------------------------------------------------

using namespace MFA::AS::PBR;

// We need other overrides for easier use as well
MFA::PBR_Essence::PBR_Essence(
    std::string const & nameId,
    Mesh const & mesh,
    std::vector<std::shared_ptr<RT::GpuTexture>> textures
)
    : EssenceBase(nameId)
    , mMeshData(mesh.getMeshData())
    , mTextures(std::move(textures))
    , mVertexCount(mesh.getVertexCount())
    , mIndexCount(mesh.getIndexCount())
{
    MFA_ASSERT(mMeshData != nullptr);
    
    prepareAnimationLookupTable();

    {// Creating buffers
        std::shared_ptr<RT::BufferGroup> indicesStageBuffer = nullptr;
        std::shared_ptr<RT::BufferGroup> primitivesStageBuffer = nullptr;
        std::shared_ptr<RT::BufferGroup> unSkinnedVerticesStageBuffer = nullptr;
        std::shared_ptr<RT::BufferGroup> verticesUVsStageBuffer = nullptr;
        
        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();
        prepareIndicesBuffer(commandBuffer, mesh, indicesStageBuffer);
        preparePrimitiveBuffer(commandBuffer, primitivesStageBuffer);
        prepareUnSkinnedVerticesBuffer(commandBuffer, mesh, unSkinnedVerticesStageBuffer);
        prepareVerticesUVsBuffer(commandBuffer, mesh, verticesUVsStageBuffer);
        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::PBR_Essence::~PBR_Essence() = default;

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::preparePrimitiveBuffer(
    VkCommandBuffer commandBuffer,
    std::shared_ptr<RT::BufferGroup> & outStageBuffer
)
{
    size_t primitiveCount = 0;
    for (auto const & subMesh : mMeshData->subMeshes) {
        primitiveCount += subMesh.primitives.size();
    }
    if (primitiveCount > 0) {
        size_t const bufferSize = sizeof(PrimitiveInfo) * primitiveCount;

        mPrimitivesBuffer = RF::CreateLocalUniformBuffer(bufferSize, 1);

        outStageBuffer = RF::CreateStageBuffer(bufferSize, 1);

        {// Updating stage buffer
            auto const mappedMemory = RF::MapHostVisibleMemory(
                outStageBuffer->buffers[0]->memory,
                0,
                bufferSize
            );
            auto * primitiveData = mappedMemory->getPtr<PrimitiveInfo>();

            for (auto const & subMesh : mMeshData->subMeshes) {
                for (auto const & primitive : subMesh.primitives) {
                    // Copy primitive into primitive info
                    PrimitiveInfo & primitiveInfo = primitiveData[primitive.uniqueId];
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
        }
        
        RF::UpdateLocalBuffer(
            commandBuffer,
            *mPrimitivesBuffer->buffers[0],
            *outStageBuffer->buffers[0]
        );
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::prepareAnimationLookupTable()
{
    int animationIndex = 0;
    for (auto const & animation : mMeshData->animations) {
        MFA_ASSERT(mAnimationNameLookupTable.find(animation.name) == mAnimationNameLookupTable.end());
        mAnimationNameLookupTable[animation.name] = animationIndex;
        ++animationIndex;
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::prepareUnSkinnedVerticesBuffer(
    VkCommandBuffer commandBuffer,
    Mesh const & mesh,
    std::shared_ptr<RT::BufferGroup> & outStageBuffer
)
{
    auto const bufferSize = sizeof(UnSkinnedVertex) * mVertexCount;
    mUnSkinnedVerticesBuffer = RF::CreateLocalStorageBuffer(bufferSize, 1);
    outStageBuffer = RF::CreateStageBuffer(bufferSize, 1);

    {// Updating stage buffer
        auto const mappedData = RF::MapHostVisibleMemory(
            outStageBuffer->buffers[0]->memory,
            0,
            bufferSize
        );
        MFA_ASSERT(mappedData != nullptr);

        auto * rawVertices = mappedData->getPtr<UnSkinnedVertex>();
        auto const * assetVertices = mesh.getVertexData()->memory.as<AS::PBR::Vertex>();
        for (uint32_t i = 0; i < mVertexCount; ++i)
        {
            auto const & assetVertex = assetVertices[i];
            auto & rawVertex = rawVertices[i];

            Copy<3>(rawVertex.localPosition, assetVertex.position);
            rawVertex.localPosition[3] = 1.0f;

            Copy<4>(rawVertex.tangent, assetVertex.tangentValue);
            Copy<3>(rawVertex.normal, assetVertex.normalValue);
            rawVertex.hasSkin = assetVertex.hasSkin;
            Copy<4>(rawVertex.jointIndices, assetVertex.jointIndices);
            Copy<4>(rawVertex.jointWeights, assetVertex.jointWeights);
        }
    }

    RF::UpdateLocalBuffer(
        commandBuffer,
        *mUnSkinnedVerticesBuffer->buffers[0],
        *outStageBuffer->buffers[0]
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::prepareVerticesUVsBuffer(
    VkCommandBuffer commandBuffer,
    AS::PBR::Mesh const & mesh,
    std::shared_ptr<RT::BufferGroup> & outStageBuffer
)
{
    auto const bufferSize = sizeof(VertexUVs) * mVertexCount;
    mVerticesUVsBuffer = RF::CreateVertexBuffer(bufferSize);
    outStageBuffer = RF::CreateStageBuffer(bufferSize, 1);

    {// Updating stage buffer
        auto const mappedData = RF::MapHostVisibleMemory(
            outStageBuffer->buffers[0]->memory,
            0,
            bufferSize
        );
        MFA_ASSERT(mappedData != nullptr);

        auto * verticesUVs = mappedData->getPtr<VertexUVs>();
        auto const * assetVertices = mesh.getVertexData()->memory.as<AS::PBR::Vertex>();
        for (uint32_t i = 0; i < mVertexCount; ++i)
        {
            auto const & assetVertex = assetVertices[i];
            auto & vertexUV = verticesUVs[i];

            Copy<2>(vertexUV.baseColorTexCoord, assetVertex.baseColorUV);
            Copy<2>(vertexUV.metallicRoughnessTexCoord, assetVertex.metallicUV);
            Copy<2>(vertexUV.normalTexCoord, assetVertex.normalMapUV);
            Copy<2>(vertexUV.emissiveTexCoord, assetVertex.emissionUV);
            Copy<2>(vertexUV.occlusionTexCoord, assetVertex.occlusionUV);
        }
    }

    RF::UpdateLocalBuffer(
        commandBuffer,
        *mVerticesUVsBuffer,
        *outStageBuffer->buffers[0]
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::prepareIndicesBuffer(
    VkCommandBuffer commandBuffer,
    AS::PBR::Mesh const & mesh,
    std::shared_ptr<RT::BufferGroup> & outStageBuffer
)
{
    auto const bufferSize = sizeof(AS::Index) * mIndexCount;
    mIndicesBuffer = RF::CreateIndexBuffer(bufferSize);
    outStageBuffer = RF::CreateStageBuffer(bufferSize, 1);

    RF::UpdateHostVisibleBuffer(
        *outStageBuffer->buffers[0],
        mesh.getIndexData()->memory
    );

    RF::UpdateLocalBuffer(
        commandBuffer,
        *mIndicesBuffer,
        *outStageBuffer->buffers[0]
    );
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

void MFA::PBR_Essence::createGraphicDescriptorSet(
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    RT::GpuTexture const & errorTexture
)
{
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
        MFA_ASSERT(mTextures.size() <= 64);
        // We need to keep imageInfos alive
        std::vector<VkDescriptorImageInfo> imageInfos{};
        for (auto const & texture : mTextures)
        {
            imageInfos.emplace_back(VkDescriptorImageInfo{
                .sampler = nullptr,
                .imageView = texture->imageView->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
        for (auto i = static_cast<uint32_t>(mTextures.size()); i < 64; ++i)
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

void MFA::PBR_Essence::createComputeDescriptorSet(
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout
)
{
    mComputeDescriptorSet = RF::CreateDescriptorSets(
        descriptorPool,
        1,
        descriptorSetLayout
    );
    
    auto const & descriptorSet = mComputeDescriptorSet.descriptorSets[0];
    MFA_VK_VALID_ASSERT(descriptorSet);

    DescriptorSetSchema descriptorSetSchema{ descriptorSet };

    /////////////////////////////////////////////////////////////////
    // Compute shader
    /////////////////////////////////////////////////////////////////

    // UnSkinnedVertices
    VkDescriptorBufferInfo const unSkinnedVerticesBufferInfo{
        .buffer = mUnSkinnedVerticesBuffer->buffers[0]->buffer,
        .offset = 0,
        .range = mUnSkinnedVerticesBuffer->bufferSize,
    };
    descriptorSetSchema.AddStorageBuffer(&unSkinnedVerticesBufferInfo);

    descriptorSetSchema.UpdateDescriptorSets();
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindForGraphicPipeline(RT::CommandRecordState const & recordState) const
{
    bindGraphicDescriptorSet(recordState);
    bindVertexUVsBuffer(recordState);
    bindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindForComputePipeline(RT::CommandRecordState const & recordState) const
{
    bindComputeDescriptorSet(recordState);
}

//-------------------------------------------------------------------------------------------------

uint32_t MFA::PBR_Essence::getVertexCount() const
{
    return mVertexCount;
}

//-------------------------------------------------------------------------------------------------

uint32_t MFA::PBR_Essence::getIndexCount() const
{
    return mIndexCount;
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindVertexUVsBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindVertexBuffer(recordState, *mVerticesUVsBuffer, 1);
}

//-------------------------------------------------------------------------------------------------

void MFA::PBR_Essence::bindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mIndicesBuffer);
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

void MFA::PBR_Essence::bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const
{
    RF::BindDescriptorSet(
        recordState,
        RF::UpdateFrequency::PerFrame,
        mComputeDescriptorSet.descriptorSets[0]
    );
}

//-------------------------------------------------------------------------------------------------
