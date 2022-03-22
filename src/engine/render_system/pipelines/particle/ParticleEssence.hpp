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

        // Pre compute barrier

        // Pre-render barrier

        void render(RT::CommandRecordState const & recordState) const;

        void updateParamsBuffer(AssetSystem::Particle::Params const & params) const;

        void bindComputeDescriptorSet(RT::CommandRecordState const & recordState) const;

    protected:

        explicit ParticleEssence(
            std::string nameId,
            uint32_t maxInstanceCount,
            std::vector<std::shared_ptr<RT::GpuTexture>> textures
        );

        void init(
            uint32_t indexCount,
            AssetSystem::Particle::Params const & params,
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

        void checkIfUpdateIsRequired(VariantsList const & variants);

    protected:

        bool mShouldUpdate = false; // We only have to update if variants are visible

        std::shared_ptr<RT::BufferGroup> mVertexBuffer = nullptr;   // StorageBuffer | VertexBuffer     // Only 1
        std::shared_ptr<RT::BufferAndMemory> mIndexBuffer = nullptr;                                    // Only 1
        std::shared_ptr<RT::BufferGroup> mInstanceBuffer = nullptr; // VertexBuffer                     // Per frame

        std::shared_ptr<RT::BufferGroup> mParamsBuffer = nullptr;

        uint32_t const mMaxInstanceCount;
        uint32_t mIndexCount = 0;

        std::vector<std::shared_ptr<RT::GpuTexture>> const mTextures;

        RT::DescriptorSetGroup mGraphicDescriptorSet {};
        RT::DescriptorSetGroup mComputeDescriptorSet {};

    private:

        std::shared_ptr<SmartBlob> mInstanceDataMemory {};
        
        std::shared_ptr<RT::BufferGroup> mInstancesBuffer {};

        uint32_t mNextDrawInstanceCount = 0;

        bool mIsInitialized = false;

    };
}
