#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/asset_system/AssetTypes.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA
{
    namespace AssetSystem
    {
        namespace PBR
        {
            struct MeshData;
        }
    }

    class PBR_Essence;
    class Scene;
    class DirectionalLightShadowResources;
    class DirectionalLightShadowRenderPass;
    class PointLightShadowRenderPass;
    class PointLightShadowResources;
    class OcclusionRenderPass;
    class PBR_Variant;
    class DepthPrePass;
    
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
            alignas(4) int lightIndex = 0;
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

        void OnResize() override;

        void CreateEssenceWithoutModel(
            std::shared_ptr<RT::GpuModel> const & gpuModel,
            std::shared_ptr<AssetSystem::PBR::MeshData> const & meshData
        ) const;
        
    protected:

        std::shared_ptr<EssenceBase> internalCreateEssence(
            std::shared_ptr<RT::GpuModel> const & gpuModel,
            std::shared_ptr<AssetSystem::MeshBase> const & cpuMesh
        ) override;

        std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;
        
    private:

        void updateVariants(float deltaTimeInSec, RT::CommandRecordState const & recordState) const;

        void createPerFrameDescriptorSets();

        void createEssenceDescriptorSets(PBR_Essence & essence) const;

        void createVariantDescriptorSets(PBR_Variant & variant) const;

        void createPerFrameDescriptorSetLayout();

        void createPerEssenceDescriptorSetLayout();

        void createPerVariantDescriptorSetLayout();
        
        void createDisplayPassPipeline();

        void createDirectionalLightShadowPassPipeline();

        void createPointLightShadowPassPipeline();

        void createDepthPassPipeline();

        void createOcclusionQueryPool();

        void destroyOcclusionQueryPool();

        void createOcclusionQueryPipeline();
        
        void createUniformBuffers();

        void retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState);

        void performDepthPrePass(RT::CommandRecordState & recordState) const;

        void renderForDepthPrePass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        void performDirectionalLightShadowPass(RT::CommandRecordState & recordState) const;

        void renderForDirectionalLightShadowPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        void performPointLightShadowPass(RT::CommandRecordState & recordState) const;

        void renderForPointLightShadowPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        void performOcclusionQueryPass(RT::CommandRecordState & recordState);

        void renderForOcclusionQueryPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode);

        void performDisplayPass(RT::CommandRecordState & recordState);

        void renderForDisplayPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        std::shared_ptr<RT::GpuTexture> mErrorTexture = nullptr;
        std::shared_ptr<RT::UniformBufferGroup> mErrorBuffer{};

        std::shared_ptr<RT::PipelineGroup> mDisplayPassPipeline{};
        
        std::shared_ptr<RT::PipelineGroup> mPointLightShadowPipeline {};
        std::unique_ptr<PointLightShadowRenderPass> mPointLightShadowRenderPass;
        std::unique_ptr<PointLightShadowResources> mPointLightShadowResources {};

        std::shared_ptr<RT::PipelineGroup> mDirectionalLightShadowPipeline {};
        std::unique_ptr<DirectionalLightShadowRenderPass> mDirectionalLightShadowRenderPass;
        std::unique_ptr<DirectionalLightShadowResources> mDirectionalLightShadowResources;

        std::shared_ptr<RT::PipelineGroup> mDepthPassPipeline{};
        std::unique_ptr<DepthPrePass> mDepthPrePass;

        std::shared_ptr<RT::PipelineGroup> mOcclusionQueryPipeline{};
        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;

        struct OcclusionQueryData
        {
            std::vector<std::weak_ptr<VariantBase>> Variants {};
            std::vector<uint64_t> Results{};
            VkQueryPool Pool{};
        };
        std::vector<OcclusionQueryData> mOcclusionQueryDataList{}; // We need one per frame

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerFrameDescriptorSetLayout{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerEssenceDescriptorSetLayout{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mPerVariantDescriptorSetLayout{};

        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts{};

        RT::DescriptorSetGroup mPerFrameDescriptorSetGroup{};

        Scene * mAttachedScene = nullptr;

    };

};
