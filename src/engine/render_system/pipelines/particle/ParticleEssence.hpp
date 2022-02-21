#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"

namespace MFA
{

    class VariantBase;

    class ParticleEssence final : public EssenceBase
    {
    public:

        using VariantsList = std::vector<std::shared_ptr<VariantBase>>;

        explicit ParticleEssence(
            std::shared_ptr<AS::Model> const & cpuModel,
            std::string const & name
        );
        explicit ParticleEssence(
            std::shared_ptr<RT::GpuModel> gpuModel,
            std::shared_ptr<AS::Particle::Mesh> mesh
        );

        ~ParticleEssence() override;
        
        ParticleEssence & operator= (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence (ParticleEssence const &) noexcept = delete;
        ParticleEssence (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence & operator = (ParticleEssence const &) noexcept = delete;

        void update(
            float deltaTimeInSec,
            VariantsList const & variants
        );
        
        void draw(
            RT::CommandRecordState const & recordState,
            float deltaTime
        ) const;

        void bindVertexBuffer(RT::CommandRecordState const & recordState) const override;

    private:

        void updateInstanceData(VariantsList const & variants);

        void updateInstanceBuffer(RT::CommandRecordState const & recordState) const;

        void updateVertexBuffer(RT::CommandRecordState const & recordState) const;

        void bindInstanceBuffer(RT::CommandRecordState const & recordState) const;

        std::shared_ptr<AS::Particle::Mesh> mMesh {};

        std::shared_ptr<SmartBlob> mInstanceDataMemory {};
        AS::Particle::PerInstanceData * mInstancesData = nullptr;

        std::vector<std::shared_ptr<RT::BufferAndMemory>> mInstancesBuffers {};
        std::shared_ptr<RT::BufferAndMemory> mInstanceStageBuffer {};

        uint32_t mNextDrawInstanceCount = 0;

    };
}
