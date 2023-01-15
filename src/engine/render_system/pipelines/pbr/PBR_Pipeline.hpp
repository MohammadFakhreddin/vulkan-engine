#pragma once

#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"

namespace MFA
{
    class PBR_Pipeline : public BasePipeline
    {
    public:

        struct DisplayPassPushConstants
        {
            uint32_t primitiveIndex = 0;      // Unique id
            float cameraPosition[3]{};
            float projectFarToNearDistance = 0.0f;
            float placeholder[3]{};
        };

        // MaxSets is the maximum number of instances that this pipeline should support (All the memory is pre-allocated)
        explicit PBR_Pipeline(std::shared_ptr<DisplayRenderPass> renderPass, uint32_t maxSets = 10000);

        ~PBR_Pipeline() override;

        std::shared_ptr<RT::DescriptorSetLayoutGroup> CreatePerFrameDescriptorSetLayout();

        std::shared_ptr<RT::DescriptorSetLayoutGroup> CreatePerEssenceDescriptorSetLayout();

    private:

        void CreatePipeline();

        void CreateErrorTexture();

        std::shared_ptr<DisplayRenderPass> mRenderPass = nullptr;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        std::shared_ptr<RT::GpuTexture> mErrorTexture = nullptr;
        std::shared_ptr<RT::BufferGroup> mErrorBuffer{};

        std::shared_ptr<RT::PipelineGroup> mPipeline{};

    };
}
