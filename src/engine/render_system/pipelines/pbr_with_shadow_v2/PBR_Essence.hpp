#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"

#include <unordered_map>

namespace MFA {

class PBR_Essence final : public EssenceBase {
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];

        float emissiveFactor[3];
        float placeholder0;

        int baseColorTextureIndex;
        float metallicFactor;
        float roughnessFactor;
        int metallicRoughnessTextureIndex;

        int normalTextureIndex;
        int emissiveTextureIndex;
        int hasSkin;
        int occlusionTextureIndex;

        int alphaMode;
        float alphaCutoff;
        float placeholder1;
        float placeholder2;
    };

    struct VertexUVs
    {
        float baseColorTexCoord[2] {};  
        float metallicRoughnessTexCoord[2] {};
        
        float normalTexCoord[2] {};
        
        float emissiveTexCoord[2] {};
        float occlusionTexCoord[2] {};
    };

    struct UnSkinnedVertex
    {
        float localPosition[4] {};

        float tangent[4] {};
        
        float normal[3] {};
        int hasSkin = -1;

        int jointIndices[4] {};

        float jointWeights[4] {}; 
    };

    // Reuse of gpu buffers are not possible because each pipeline has its own layout
    explicit PBR_Essence(
        std::string const & nameId,
        AS::PBR::Mesh const & mesh,
        std::vector<std::shared_ptr<RT::GpuTexture>> textures
    );
    ~PBR_Essence() override;
    
    PBR_Essence & operator= (PBR_Essence && rhs) noexcept = delete;
    PBR_Essence (PBR_Essence const &) noexcept = delete;
    PBR_Essence (PBR_Essence && rhs) noexcept = delete;
    PBR_Essence & operator = (PBR_Essence const &) noexcept = delete;

    void prepareAnimationLookupTable();

    void preparePrimitiveBuffer(
        VkCommandBuffer commandBuffer,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    );

    void prepareUnSkinnedVerticesBuffer(
        VkCommandBuffer commandBuffer,
        AS::PBR::Mesh const & mesh,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    );

    void prepareVerticesUVsBuffer(
        VkCommandBuffer commandBuffer,
        AS::PBR::Mesh const & mesh,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    );

    void prepareIndicesBuffer(
        VkCommandBuffer commandBuffer,
        AS::PBR::Mesh const & mesh,
        std::shared_ptr<RT::BufferGroup> & outStageBuffer
    );

    [[nodiscard]]
    int getAnimationIndex(char const * name) const noexcept;
    
    [[nodiscard]]
    AS::PBR::MeshData const * getMeshData() const;

    void createGraphicDescriptorSet(
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        RT::GpuTexture const & errorTexture
    );

    void createComputeDescriptorSet(
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout
    );

    void bindForGraphicPipeline(RT::CommandRecordState const & recordState) const;

    void bindForComputePipeline(RT::CommandRecordState const & recordState) const;

    [[nodiscard]]
    uint32_t getVertexCount() const;

    [[nodiscard]]
    uint32_t getIndexCount() const;

private:

    void bindVertexUVsBuffer(RT::CommandRecordState const & recordState) const;

    void bindIndexBuffer(RT::CommandRecordState const & recordState) const;

    void bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const;

    void bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const;
    
    RT::DescriptorSetGroup mGraphicDescriptorSet {};

    RT::DescriptorSetGroup mComputeDescriptorSet {};

    std::shared_ptr<RT::BufferGroup> mPrimitivesBuffer = nullptr;
    
    std::unordered_map<std::string, int> mAnimationNameLookupTable {};

    std::shared_ptr<AS::PBR::MeshData> mMeshData {};

    std::vector<std::shared_ptr<RT::GpuTexture>> mTextures {};

    std::shared_ptr<RT::BufferGroup> mVerticesUVsBuffer {};

    std::shared_ptr<RT::BufferGroup> mUnSkinnedVerticesBuffer {};

    std::shared_ptr<RT::BufferGroup> mIndicesBuffer {};

    uint32_t const mVertexCount;

    uint32_t const mIndexCount;
};

}
