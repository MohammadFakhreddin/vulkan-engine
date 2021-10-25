#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

#include "glm/glm.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA
{

    class OcclusionRenderPass;
    class ShadowRenderPassV2;
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

        struct ShadowPassVertexStagePushConstants
        {
            float modelTransform[16];
            float inverseNodeTransform[16];
            int faceIndex;
            int skinIndex;
            int placeholder0[2];
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

        struct ShadowViewProjectionData
        {
            float viewMatrices[6][16];
        };
        static_assert(sizeof(ShadowViewProjectionData) == 6 * 64);

        struct LightData
        {
            float lightPosition[3]; //TODO We will have multiple lights and visibility info
            float placeholder0;
            float lightColor[3];
            float placeholder1;
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

        void UpdateLightPosition(const float lightPosition[3]);

        void UpdateLightColor(const float lightColor[3]);

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

        void createOcclusionQueryPipeline();

        void destroyPipeline();

        void createUniformBuffers();

        void destroyUniformBuffers();

        void updateLightBuffer(RT::CommandRecordState const & drawPass);

        void updateShadowViewProjectionBuffer(RT::CommandRecordState const & drawPass);

        void updateShadowViewProjectionData();
        
        void retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState);

        void performDepthPrePass(RT::CommandRecordState & recordState);

        void performShadowPass(RT::CommandRecordState & recordState);

        void performOcclusionQueryPass(RT::CommandRecordState & recordState);

        void performDisplayPass(RT::CommandRecordState & recordState);

        static constexpr float SHADOW_WIDTH = 1024;
        static constexpr float SHADOW_HEIGHT = 1024;

        bool mIsInitialized = false;

        RT::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        RT::GpuTexture * mErrorTexture = nullptr;
        RT::UniformBufferCollection mErrorBuffer{};

        RT::PipelineGroup mDisplayPassPipeline{};
        RT::PipelineGroup mShadowPassPipeline{};
        RT::PipelineGroup mDepthPassPipeline{};
        RT::PipelineGroup mOcclusionQueryPipeline{};

        std::unique_ptr<ShadowRenderPassV2> mShadowRenderPass;

        std::unique_ptr<DepthPrePass> mDepthPrePass;

        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;


        RT::UniformBufferCollection mLightBuffer{};
        RT::UniformBufferCollection mShadowViewProjectionBuffer{};

        float mLightPosition[3]{};
        float mCameraPosition[3]{};
        float mLightColor[3]{};

        float mProjectionNear = 0.0f;
        float mProjectionFar = 0.0f;
        float mProjectionFarToNearDistance = 0.0f;

        glm::mat4 mShadowProjection{};

        ShadowViewProjectionData mShadowViewProjectionData{};

        uint32_t mLightBufferUpdateCounter = 0;
        uint32_t mShadowViewProjectionUpdateCounter = 0;

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
