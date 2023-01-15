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
    
        struct PushConstants
        {
            float viewportDimension [2] {};
            float placeholder[2] {};
        };

        explicit ParticlePipeline();
        ~ParticlePipeline() override;

        PIPELINE_PROPS(
            ParticlePipeline,
            EventTypes::RenderEvent | EventTypes::UpdateEvent | EventTypes::ComputeEvent,
            RenderOrder::AfterEverything
        )

        void Init() override;

        void onResize() override {}

        void Shutdown() override;
        
        void render(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

        void update(float deltaTimeInSec) override;

        void compute(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

        std::shared_ptr<EssenceBase> CreateEssence(
            std::string const & nameId,
            std::shared_ptr<AssetSystem::Model> const & cpuModel,
            std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures
        ) override;

    protected:

        void internalAddEssence(EssenceBase * essence) override;

        std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;

    private:

        void createPerFrameDescriptorSetLayout();

        void createPerEssenceGraphicDescriptorSetLayout();
        void createPerEssenceComputeDescriptorSetLayout();

        void createPerFrameDescriptorSets();
        
        void createComputePipeline();

        void createGraphicPipeline();
        
        static constexpr int MAXIMUM_TEXTURE_PER_ESSENCE = 10;
        
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerFrameDescriptorSetLayout {};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceGraphicDescriptorSetLayout {};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceComputeDescriptorSetLayout {};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup {};
        
        std::shared_ptr<RT::SamplerGroup> mSamplerGroup {};
        std::shared_ptr<RT::GpuTexture> mErrorTexture {};

        std::shared_ptr<RT::PipelineGroup> mComputePipeline {};
        std::shared_ptr<RT::PipelineGroup> mGraphicPipeline {};
    };

}
