#pragma once

#include "engine/render_system/pipelines/BasePipeline.hpp"
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

        explicit ParticlePipeline();
        ~ParticlePipeline() override;

        PIPELINE_PROPS(
            ParticlePipeline,
            EventTypes::RenderEvent | EventTypes::UpdateEvent | EventTypes::ComputeEvent,
            RenderOrder::AfterEverything
        )

        void init() override;

        void onResize() override {}

        void shutdown() override;
        
        void render(RT::CommandRecordState & recordState, float deltaTime) override;

        void update(float deltaTimeInSec) override;

        void compute(RT::CommandRecordState & recordState, float deltaTime) override;

        std::shared_ptr<EssenceBase> createEssenceWithModel(
            std::shared_ptr<AssetSystem::Model> const & cpuModel,
            std::string const & name
        );

    protected:

        void internalAddEssence(EssenceBase * essence) override;

        std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;

    private:

        void createPerFrameDescriptorSetLayout();
        void createPerEssenceDescriptorSetLayout();

        void createPerFrameDescriptorSets();
        void createPerEssenceDescriptorSets(ParticleEssence * essence) const;

        void createComputePipeline();

        void createGraphicPipeline();
        
        static constexpr int MAXIMUM_TEXTURE_PER_ESSENCE = 10;
        
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerFrameDescriptorSetLayout {};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceDescriptorSetLayout {};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup {};
        
        std::shared_ptr<RT::SamplerGroup> mSamplerGroup {};
        std::shared_ptr<RT::GpuTexture> mErrorTexture {};

        std::shared_ptr<RT::PipelineGroup> mComputePipeline {};
        std::shared_ptr<RT::PipelineGroup> mGraphicPipeline {};
    };

}
