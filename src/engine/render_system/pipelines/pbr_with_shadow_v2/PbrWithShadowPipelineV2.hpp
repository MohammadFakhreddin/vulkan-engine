#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA
{
    class PointLightShadowRenderPass;
    class OcclusionRenderPass;
    class DrawableVariant;
    class DepthPrePass;

    class PBRWithShadowPipelineV2 final : public BasePipeline
    {
    public:

        struct OcclusionPassVertexStagePushConstants
        {
            float modelTransform[16];
            float inverseNodeTransform[16];
            int skinIndex;
            int placeholder0[3];
        };

        struct PointLightShadowPassPushConstants
        {
            float modelTransform[16];
            float inverseNodeTransform[16];
            int faceIndex;
            int skinIndex;
            uint32_t lightIndex;
            int placeholder0;
        };

        struct DepthPrePassVertexStagePushConstants
        {
            float modelTransform[16];
            float inverseNodeTransform[16];
            int skinIndex;
            int placeholder0[3];
        };

        struct DisplayPassAllStagesPushConstants
        {
            float modelTransform[16];
            float inverseNodeTransform[16];
            int skinIndex;
            uint32_t primitiveIndex;      // Unique id
            int placeholder0[2];
        };

        explicit PBRWithShadowPipelineV2();

        ~PBRWithShadowPipelineV2() override;

        PBRWithShadowPipelineV2(PBRWithShadowPipelineV2 const &) noexcept = delete;
        PBRWithShadowPipelineV2(PBRWithShadowPipelineV2 &&) noexcept = delete;
        PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 const &) noexcept = delete;
        PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 &&) noexcept = delete;

        void Init(
            RT::SamplerGroup * samplerGroup,
            RT::GpuTexture * errorTexture,
            float projectionNear,
            float projectionFar
        );

        void Shutdown() override;

        void PreRender(RT::CommandRecordState & recordState, float deltaTime) override;

        void Render(RT::CommandRecordState & drawPass, float deltaTime) override;

        void PostRender(RT::CommandRecordState & drawPass, float deltaTime) override;

        void OnResize() override;
        
        std::weak_ptr<DrawableVariant> CreateDrawableVariant(char const * essenceName) override;

    protected:

        void internalCreateDrawableEssence(DrawableEssence & essence) override;

    private:

        void createFrameDescriptorSets();

        void createEssenceDescriptorSets(DrawableEssence & essence) const;

        void createVariantDescriptorSets(DrawableVariant * variant);

        void createPerFrameDescriptorSetLayout();

        void createPerEssenceDescriptorSetLayout();

        void createPerVariantDescriptorSetLayout();

        void destroyDescriptorSetLayout() const;

        void createDisplayPassPipeline();

        void createShadowPassPipeline();

        void createDepthPassPipeline();

        void createOcclusionQueryPool();

        void destroyOcclusionQueryPool();

        void createOcclusionQueryPipeline();

        void destroyPipeline();

        void createUniformBuffers();

        void destroyUniformBuffers();

        void retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState);

        void performDepthPrePass(RT::CommandRecordState & recordState);

        void performPointLightShadowPass(RT::CommandRecordState & recordState);

        void performOcclusionQueryPass(RT::CommandRecordState & recordState);

        void performDisplayPass(RT::CommandRecordState & recordState);

        bool mIsInitialized = false;

        RT::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        RT::GpuTexture * mErrorTexture = nullptr;
        RT::UniformBufferCollection mErrorBuffer{};

        RT::PipelineGroup mDisplayPassPipeline{};
        RT::PipelineGroup mPointLightShadowPipeline{};
        RT::PipelineGroup mDepthPassPipeline{};
        RT::PipelineGroup mOcclusionQueryPipeline{};

        struct PointLightShadowRenderTargets;
        std::unique_ptr<PointLightShadowRenderTargets> mPointLightShadowRenderTargets;
        std::unique_ptr<PointLightShadowRenderPass> mPointLightShadowRenderPass;
        
        std::unique_ptr<DepthPrePass> mDepthPrePass;

        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;

        float mProjectionNear = 0.0f;
        float mProjectionFar = 0.0f;
        float mProjectionFarToNearDistance = 0.0f;
        
        struct OcclusionQueryData
        {
            std::vector<DrawableVariant *> Variants{};
            std::vector<uint64_t> Results{};
            VkQueryPool Pool{};
        };
        std::vector<OcclusionQueryData> mOcclusionQueryDataList{}; // We need one per frame

        VkDescriptorSetLayout mPerFrameDescriptorSetLayout{};
        VkDescriptorSetLayout mPerEssenceDescriptorSetLayout{};
        VkDescriptorSetLayout mPerVariantDescriptorSetLayout{};

        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts{};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup{};

    };

};
