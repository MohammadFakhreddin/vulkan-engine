#pragma once

#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA
{
    namespace AssetSystem
    {
        struct Model;
    }

    class ParticleEssence;
    class Scene;

    class ParticlePipeline final : public BasePipeline
    {
    public:

        explicit ParticlePipeline(Scene * attachedScene);
        ~ParticlePipeline() override;

        PIPELINE_PROPS(ParticlePipeline)

        void Init(
            std::shared_ptr<RT::SamplerGroup> const & samplerGroup,
            std::shared_ptr<RT::GpuTexture> const & errorTexture
        );

        void Shutdown() override;

        void PreRender(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

        void Render(RT::CommandRecordState & recordState, float deltaTime) override;
        
        void OnResize() override {}

        std::shared_ptr<EssenceBase> CreateEssenceWithModel(
            std::shared_ptr<AssetSystem::Model> const & cpuModel,
            std::string const & name
        );

    protected:

        std::shared_ptr<EssenceBase> internalCreateEssence(
            std::shared_ptr<RT::GpuModel> const & gpuModel,
            std::shared_ptr<AssetSystem::MeshBase> const & cpuMesh
        ) override;

        std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;

    private:

        void createPerFrameDescriptorSetLayout();
        void createPerEssenceDescriptorSetLayout();

        void createPerFrameDescriptorSets();
        void createPerEssenceDescriptorSets(ParticleEssence * essence) const;

        void createPipeline();

        static constexpr int MAXIMUM_TEXTURE_PER_ESSENCE = 10;
        
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerFrameDescriptorSetLayout {};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceDescriptorSetLayout {};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup {};
        Scene * mAttachedScene = nullptr;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup {};
        std::shared_ptr<RT::GpuTexture> mErrorTexture {};

        std::shared_ptr<RT::PipelineGroup> mPipeline {};
    };

}
