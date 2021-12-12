#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA
{
    class Scene;
    class DirectionalLightShadowResources;
    class DirectionalLightShadowRenderPass;
    class PointLightShadowRenderPass;
    class OcclusionRenderPass;
    class DrawableVariant;
    class DepthPrePass;
    class PointLightShadowResources;

    class PBRWithShadowPipelineV2 final : public BasePipeline
    {
    public:

        struct OcclusionPassPushConstants
        {
            alignas(64) float modelTransform[16] {};
            alignas(64) float inverseNodeTransform[16] {};
            alignas(4) int skinIndex = 0;
            alignas(4) uint32_t primitiveIndex = 0;      // Unique id
            alignas(4) int placeholder1 = 0;
            alignas(4) int placeholder2 = 0;

        };
        
        struct DirectionalLightShadowPassPushConstants
        {   
            alignas(64) float model[16] {};
            alignas(64) float inverseNodeTransform[16] {};
            alignas(4) int skinIndex = 0;
            alignas(4) int placeholder0 = 0;
            alignas(4) int placeholder1 = 0;
            alignas(4) int placeholder2 = 0;
        };

        struct PointLightShadowPassPushConstants
        {
            alignas(64) float modelTransform[16] {};
            alignas(64) float inverseNodeTransform[16] {};
            alignas(4) int skinIndex = 0;
            alignas(4) int placeholder0 = 0;
            alignas(4) int placeholder1 = 0;
            alignas(4) int placeholder2 = 0;
        };

        struct DepthPrePassPushConstants
        {
            alignas(64) float modelTransform[16] {};
            alignas(64) float inverseNodeTransform[16] {};
            alignas(4) int skinIndex = 0;
            alignas(4) uint32_t primitiveIndex = 0;      // Unique id
            alignas(4) int placeholder0 = 0;
            alignas(4) int placeholder1 = 0;
        };

        struct DisplayPassPushConstants
        {
            alignas(64) float modelTransform[16] {};
            alignas(64) float inverseNodeTransform[16] {};
            alignas(4) int skinIndex = 0;
            alignas(4) uint32_t primitiveIndex = 0;      // Unique id
            alignas(4) int placeholder0 = 0;
            alignas(4) int placeholder1 = 0;
        };

        explicit PBRWithShadowPipelineV2(Scene * attachedScene);
        ~PBRWithShadowPipelineV2() override;

        PIPELINE_PROPS(PBRWithShadowPipelineV2);

        void Init(
            std::shared_ptr<RT::SamplerGroup> samplerGroup,
            std::shared_ptr<RT::GpuTexture> errorTexture
        );

        void Shutdown() override;

        void PreRender(RT::CommandRecordState & recordState, float deltaTime) override;

        void Render(RT::CommandRecordState & recordState, float deltaTime) override;

        void PostRender(RT::CommandRecordState & drawPass, float deltaTime) override;

        void OnResize() override;
        
        DrawableVariant * CreateDrawableVariant(RT::GpuModelId id) override;

    protected:

        void internalCreateDrawableEssence(DrawableEssence & essence) override;

    private:

        void createPerFrameDescriptorSets();

        void createEssenceDescriptorSets(DrawableEssence & essence) const;

        void createVariantDescriptorSets(DrawableVariant * variant);

        void createPerFrameDescriptorSetLayout();

        void createPerEssenceDescriptorSetLayout();

        void createPerVariantDescriptorSetLayout();

        void destroyDescriptorSetLayout() const;

        void createDisplayPassPipeline();

        void createDirectionalLightShadowPassPipeline();

        void createPointLightShadowPassPipeline();

        void createDepthPassPipeline();

        void createOcclusionQueryPool();

        void destroyOcclusionQueryPool();

        void createOcclusionQueryPipeline();

        void destroyPipeline();

        void createUniformBuffers();

        void retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState);

        void performDepthPrePass(RT::CommandRecordState & recordState);

        void performDirectionalLightShadowPass(RT::CommandRecordState & recordState);

        void performPointLightShadowPass(RT::CommandRecordState & recordState);

        void performOcclusionQueryPass(RT::CommandRecordState & recordState);

        void performDisplayPass(RT::CommandRecordState & recordState);

        bool mIsInitialized = false;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        std::shared_ptr<RT::GpuTexture> mErrorTexture = nullptr;
        std::shared_ptr<RT::UniformBufferGroup> mErrorBuffer{};

        RT::PipelineGroup mDisplayPassPipeline{};
        
        RT::PipelineGroup mPointLightShadowPipeline {};
        std::unique_ptr<PointLightShadowRenderPass> mPointLightShadowRenderPass;
        std::unique_ptr<PointLightShadowResources> mPointLightShadowResources {};

        RT::PipelineGroup mDirectionalLightShadowPipeline {};
        std::unique_ptr<DirectionalLightShadowRenderPass> mDirectionalLightShadowRenderPass;
        std::unique_ptr<DirectionalLightShadowResources> mDirectionalLightShadowResources;

        RT::PipelineGroup mDepthPassPipeline{};
        std::unique_ptr<DepthPrePass> mDepthPrePass;

        RT::PipelineGroup mOcclusionQueryPipeline{};
        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;

        struct OcclusionQueryData
        {
            std::vector<std::weak_ptr<DrawableVariant>> Variants {};
            std::vector<uint64_t> Results{};
            VkQueryPool Pool{};
        };
        std::vector<OcclusionQueryData> mOcclusionQueryDataList{}; // We need one per frame

        VkDescriptorSetLayout mPerFrameDescriptorSetLayout{};
        VkDescriptorSetLayout mPerEssenceDescriptorSetLayout{};
        VkDescriptorSetLayout mPerVariantDescriptorSetLayout{};

        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts{};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup{};

        Scene * mAttachedScene = nullptr;

    };

};
