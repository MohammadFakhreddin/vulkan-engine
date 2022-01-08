#pragma once

#include "engine/render_system/pipelines/BasePipeline.hpp"

namespace MFA
{
    class ParticleEssence;
    class Scene;

    class ParticlePipeline final : public BasePipeline
    {
    public:
        struct PushConstants
        {
            float model[16] {};
        };

        explicit ParticlePipeline(Scene * attachedScene);
        ~ParticlePipeline() override;

        PIPELINE_PROPS(ParticlePipeline)

        void Init(
            std::shared_ptr<RT::SamplerGroup> samplerGroup,
            std::shared_ptr<RT::GpuTexture> errorTexture
        );

        void Shutdown() override;

        void PreRender(RT::CommandRecordState & drawPass, float deltaTime) override;

        void Render(RT::CommandRecordState & drawPass, float deltaTime) override;
        
        void OnResize() override {}
        
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
        
        VkDescriptorSetLayout mPerFrameDescriptorSetLayout {};
        VkDescriptorSetLayout mPerEssenceDescriptorSetLayout {};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup {};
        Scene * mAttachedScene = nullptr;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup {};
        std::shared_ptr<RT::GpuTexture> mErrorTexture {};

        RT::PipelineGroup mPipeline {};
    };

}
