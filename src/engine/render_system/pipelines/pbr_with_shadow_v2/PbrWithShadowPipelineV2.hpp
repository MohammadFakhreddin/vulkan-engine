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

        struct DisplayViewProjectionData
        {
            alignas(64) float projection[16];
        };
        static_assert(sizeof(DisplayViewProjectionData) == 64);

        struct ShadowViewProjectionData
        {
            float viewMatrices[6][16];
        };

        struct OcclusionPassVertexStagePushConstants
        {
            float modelTransform[16];
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

        struct DisplayLightAndCameraData
        {
            alignas(16) float lightPosition[3];
            alignas(16) float cameraPosition[3];
            alignas(16) float lightColor[3];
            alignas(4) float projectFarToNearDistance;
        };

        struct ShadowLightData
        {
            alignas(16) float lightPosition[4];
            alignas(4) float projectionMatrixDistance;
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

        void PreRender(RT::CommandRecordState & drawPass, float deltaTime) override;

        void Render(RT::CommandRecordState & drawPass, float deltaTime) override;

        void OnResize() override {}

        void UpdateCameraView(const float view[16]);

        void UpdateCameraProjection(const float projection[16]);

        void UpdateLightPosition(const float lightPosition[3]);

        void UpdateCameraPosition(const float cameraPosition[3]);

        void UpdateLightColor(const float lightColor[3]);

        DrawableVariant * CreateDrawableVariant(char const * essenceName) override;

    private:

        void createDisplayPassDescriptorSets(DrawableVariant * variant);

        void createShadowPassDescriptorSets(DrawableVariant * variant);

        void createDepthPassDescriptorSets(DrawableVariant * variant);

        void createOcclusionPassDescriptorSets();

        void createDisplayPassDescriptorSetLayout();

        void createShadowPassDescriptorSetLayout();

        void createDepthDescriptorSetLayout();

        void createOcclusionPassDescriptorSetLayout();

        void destroyDescriptorSetLayout() const;

        void createDisplayPassPipeline();

        void createShadowPassPipeline();

        void createDepthPassPipeline();

        void createOcclusionQueryPipeline();

        void destroyPipeline();

        void createUniformBuffers();

        void destroyUniformBuffers();

        void updateDisplayLightBuffer(RT::CommandRecordState const & drawPass);

        void updateShadowLightBuffer(RT::CommandRecordState const & drawPass);

        void updateShadowViewProjectionBuffer(RT::CommandRecordState const & drawPass);

        void updateShadowViewProjectionData();

        void updateDisplayViewProjectionBuffer(RT::CommandRecordState const & drawPass);

        static constexpr float SHADOW_WIDTH = 1024;
        static constexpr float SHADOW_HEIGHT = 1024;
        
        bool mIsInitialized = false;

        RT::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        RT::GpuTexture * mErrorTexture = nullptr;
        RT::UniformBufferGroup mErrorBuffer{};

        VkDescriptorSetLayout mDisplayPassDescriptorSetLayout{};
        RT::PipelineGroup mDisplayPassPipeline{};
        RT::UniformBufferGroup mDisplayLightAndCameraBuffer{};
        RT::UniformBufferGroup mDisplayViewProjectionBuffer{};

        VkDescriptorSetLayout mShadowPassDescriptorSetLayout{};
        RT::PipelineGroup mShadowPassPipeline{};
        RT::UniformBufferGroup mShadowLightBuffer{};

        VkDescriptorSetLayout mDepthPassDescriptorSetLayout{};
        RT::PipelineGroup mDepthPassPipeline{};

        RT::PipelineGroup mOcclusionQueryPipeline {};

        std::unique_ptr<ShadowRenderPassV2> mShadowRenderPass;

        std::unique_ptr<DepthPrePass> mDepthPrePass;

        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;

        RT::UniformBufferGroup mShadowViewProjectionBuffer{};

        float mLightPosition[3]{};
        float mCameraPosition[3]{};
        float mLightColor[3]{};

        float mProjectionNear = 0.0f;
        float mProjectionFar = 0.0f;
        float mProjectionFarToNearDistance = 0.0f;

        glm::mat4 mShadowProjection{};

        ShadowViewProjectionData mShadowViewProjectionData{};

        float mDisplayProjection[16]{};

        float mDisplayView[16]{};

        uint32_t mDisplayViewProjectionNeedUpdate = 0;
        uint32_t mDisplayLightNeedUpdate = 0;
        uint32_t mShadowLightNeedUpdate = 0;
        uint32_t mShadowViewProjectionNeedUpdate = 0;

        struct OcclusionQueryData {
            std::vector<DrawableVariant *> Variants {};
            std::vector<uint64_t> Results {};
            VkQueryPool Pool {};
        };
        std::vector<OcclusionQueryData> mOcclusionQueryDataList {}; // We need one per frame

        VkDescriptorSetLayout mOcclusionDescriptorSetLayout {};
        RT::DescriptorSetGroup mOcclusionDescriptorSetGroup {}; 

        
    };

};
