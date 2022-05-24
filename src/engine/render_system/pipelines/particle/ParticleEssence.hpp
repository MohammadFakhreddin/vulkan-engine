#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"

namespace MFA
{

    class VariantBase;

    class ParticleEssence : public EssenceBase
    {
    public:

        using VariantsList = std::vector<std::shared_ptr<VariantBase>>;

        ~ParticleEssence() override;
        
        ParticleEssence & operator= (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence (ParticleEssence const &) noexcept = delete;
        ParticleEssence (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence & operator = (ParticleEssence const &) noexcept = delete;

        void update(VariantsList const & variants);

        void preComputeBarrier(std::vector<VkBufferMemoryBarrier> & outBarrier) const;

        void compute(RT::CommandRecordState const & recordState);

        void preRenderBarrier(std::vector<VkBufferMemoryBarrier> & outBarrier) const;

        void render(RT::CommandRecordState const & recordState) const;

        void notifyParamsBufferUpdated();

        void createGraphicDescriptorSet(
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout,
            RT::GpuTexture const & errorTexture,
            uint32_t maxTextureCount
        );

        void createComputeDescriptorSet(
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout
        );

    protected:

        explicit ParticleEssence(
            std::string nameId,
            AS::Particle::Params const & params,
            uint32_t maxInstanceCount,
            std::vector<std::shared_ptr<RT::GpuTexture>> textures
        );

        void init(
            CBlob const & vertexData,
            CBlob const & indexData
        );

    private:

        void updateInstanceData(VariantsList const & variants);

        void updateInstanceBuffer(RT::CommandRecordState const & recordState) const;

        void bindVertexBuffer(RT::CommandRecordState const & recordState) const;

        void bindInstanceBuffer(RT::CommandRecordState const & recordState) const;

        void bindIndexBuffer(RT::CommandRecordState const & recordState) const;

        void bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const;

        void bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const;

        void checkIfUpdateIsRequired(VariantsList const & variants);

        void updateParamsBuffer(RT::CommandRecordState const & recordState) const;

        void createVertexBuffer(
            VkCommandBuffer commandBuffer,
            CBlob const & vertexBlob,
            std::shared_ptr<RT::BufferGroup> & outStageBuffer
        );

        void createIndexBuffer(
            VkCommandBuffer commandBuffer,
            CBlob const & indexBlob,
            std::shared_ptr<RT::BufferGroup> & outStageBuffer
        );

        void createInstanceBuffer();

        void createParamsBuffer(
            VkCommandBuffer commandBuffer,
            std::shared_ptr<RT::BufferGroup> & outStageBuffer
        );

    protected:

        bool mShouldUpdate = false; // We only have to update if variants are visible

        std::shared_ptr<RT::BufferGroup> mVertexBuffer = nullptr;   // StorageBuffer | VertexBuffer     // Only 1
        std::shared_ptr<RT::BufferAndMemory> mIndexBuffer = nullptr;                                    // Only 1
        std::shared_ptr<RT::BufferGroup> mInstanceBuffer = nullptr; // VertexBuffer                     // Per frame

        AS::Particle::Params mParams {};

        std::shared_ptr<RT::BufferGroup> mParamsBuffer = nullptr;

        uint32_t const mMaxInstanceCount;
        
        std::vector<std::shared_ptr<RT::GpuTexture>> const mTextures;

        RT::DescriptorSetGroup mGraphicDescriptorSet {};
        RT::DescriptorSetGroup mComputeDescriptorSet {};

    private:

        std::shared_ptr<SmartBlob> mInstanceData {};
        
        uint32_t mNextDrawInstanceCount = 0;

        bool mIsInitialized = false;

        bool mIsParamsBufferDirty = false;

    };
}
