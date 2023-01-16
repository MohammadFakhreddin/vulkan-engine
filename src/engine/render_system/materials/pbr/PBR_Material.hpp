#pragma once

#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/render_system/materials/BaseMaterial.hpp"
#include "engine/render_system/render_resources/directional_light_shadow/DirectionalLightShadowResource.hpp"
#include "engine/render_system/render_resources/point_light_shadow/PointLightShadowResource.hpp"

namespace MFA
{
    // Maybe I should call this material
    class PBR_Material : public BaseMaterial
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
        explicit PBR_Material(
            std::shared_ptr<DisplayRenderPass> renderPass,
            std::shared_ptr<RT::BufferGroup> cameraBufferGroup,
            std::shared_ptr<RT::BufferGroup> directionalLightBufferGroup,
            std::shared_ptr<RT::BufferGroup> pointLightBufferGroup,
            std::shared_ptr<DirectionalLightShadowResource> directionalLightShadowResource,
            std::shared_ptr<PointLightShadowResource> pointLightShadowResource,
            uint32_t maxSets = 10000
        );

        ~PBR_Material() override;

        // TODO: This function might need to change based on dependency an output to automatically detect order
        PIPELINE_PROPS(
            PBR_Material,
            EventTypes::RenderEvent,
            RenderOrder::BeforeEverything
        );

        void Render(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

        void CreatePerFrameDescriptorSetLayout();

        void CreatePerEssenceDescriptorSetLayout();

        void CreatePerFrameDescriptorSets();

    private:

        void CreatePipeline();

        void CreateErrorTexture();

        void PerformDisplayPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode);

        std::shared_ptr<DisplayRenderPass> mRenderPass = nullptr;

        std::shared_ptr<RT::BufferGroup> mCameraBufferGroup{};
        std::shared_ptr<RT::BufferGroup> mDLightBufferGroup{};
        std::shared_ptr<RT::BufferGroup> mPLightBufferGroup{};
        std::shared_ptr<DirectionalLightShadowResource> mDLightShadowResource{};
        std::shared_ptr<PointLightShadowResource> mPLightShadowResource{};

        std::shared_ptr<DirectionalLightShadowResource> mDirectionalLightShadowResource{};
        std::shared_ptr<PointLightShadowResource> mPointLightShadowResource{};

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        std::shared_ptr<RT::GpuTexture> mErrorTexture = nullptr;
        std::shared_ptr<RT::BufferGroup> mErrorBuffer{};

        std::shared_ptr<RT::PipelineGroup> mPipeline{};

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerFrameDescriptorSetLayout {};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceDescriptorSetLayout{};

        std::shared_ptr<RT::DescriptorSetGroup> mPerFrameDescriptorSet{};
        std::shared_ptr<RT::DescriptorSetGroup> mPerEssenceDescriptorSet{};

    };
}
