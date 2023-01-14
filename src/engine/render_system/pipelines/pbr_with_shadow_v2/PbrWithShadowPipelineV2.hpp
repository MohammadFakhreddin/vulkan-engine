#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA
{
    class PointLightComponent;

    namespace AssetSystem
    {
        namespace PBR
        {
            struct MeshData;
        }
    }

    class PBR_Essence;
    class Scene;
    class DirectionalLightShadowResource;
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
            float model [16] {};
        };

        struct PointLightShadowPassPushConstants
        {
            int lightIndex = 0;
            int faceIndex = 0;
            float projectFarToNearDistance = 0;
            int placeholder2 = 0;
        };

        struct DirectionalLightPushConstants
        {
            int lightIndex = 0;
            int placeholder0 = 0;
            int placeholder1 = 0;
            int placeholder2 = 0;
        };

        struct DepthPrePassPushConstants
        {
            uint32_t primitiveIndex = 0;      // Unique id
            int placeholder0 = 0;
            int placeholder1 = 0;
            int placeholder2 = 0;
        };

        struct DisplayPassPushConstants
        {
            uint32_t primitiveIndex = 0;      // Unique id
            float cameraPosition[3] {};
            float projectFarToNearDistance = 0.0f;
            float placeholder[3] {};
        };

        struct SkinningPushConstants
        {
            float model[16] {};

            float inverseNodeTransform[16] {};

            int skinIndex;
            uint32_t vertexCount;
            uint32_t vertexStartingIndex;
            int placeholder0;
        };

        explicit PBRWithShadowPipelineV2();
        ~PBRWithShadowPipelineV2() override;
        
        PIPELINE_PROPS(
            PBRWithShadowPipelineV2,
            EventTypes::PreRenderEvent |
            EventTypes::RenderEvent |
            EventTypes::UpdateEvent |
            EventTypes::ComputeEvent,
            RenderOrder::BeforeEverything
        );

        void init() override;

        void shutdown() override;

        void compute(RT::CommandRecordState & recordState, float deltaTime) override;

        void preRender(RT::CommandRecordState & recordState, float deltaTime) override;

        void render(RT::CommandRecordState & recordState, float deltaTimeInSec) override;

        void update(float deltaTimeInSec) override;

        void onResize() override;

        std::shared_ptr<EssenceBase> CreateEssence(
            std::string const & nameId,
            std::shared_ptr<AssetSystem::Model> const & cpuModel,
            std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures
        ) override;

    protected:

        void internalAddEssence(EssenceBase * essence) override;

        std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) override;
        
    private:

        void updateVariantsBuffers(RT::CommandRecordState const & recordState) const;

        void performSkinning(RT::CommandRecordState & recordState);

        void preComputeBarrier(RT::CommandRecordState const & recordState) const;

        void postComputeBarrier(RT::CommandRecordState const & recordState);

        void postRenderVariants(float deltaTimeInSec) const;

        void createGfxPerFrameDescriptorSets();

        void createGfxPerFrameDescriptorSetLayout();

        void createGfxPerEssenceDescriptorSetLayout();
        
        void createDisplayPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);

        void createDirectionalLightShadowPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);

        void createPointLightShadowPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);

        void createDepthPassPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);

        void createOcclusionQueryPool();

        void destroyOcclusionQueryPool();

        void createOcclusionQueryPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);
        
        void createUniformBuffers();

        void createSkinningPerEssenceDescriptorSetLayout();

        void createSkinningPerVariantDescriptorSetLayout();

        void createSkinningPipeline(std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts);

        void retrieveOcclusionQueryResult(RT::CommandRecordState const & recordState);

        void performDepthPrePass(RT::CommandRecordState & recordState) const;

        void renderForDepthPrePass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        void performDirectionalLightShadowPass(RT::CommandRecordState & recordState) const;

        void renderForDirectionalLightShadowPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        void performPointLightShadowPass(RT::CommandRecordState & recordState) const;

        void prepareShadowMapsForSampling(RT::CommandRecordState const & recordState) const;

        void renderForPointLightShadowPass(
            RT::CommandRecordState const & recordState,
            AS::AlphaMode alphaMode,
            PointLightShadowPassPushConstants & pushConstants,
            PointLightComponent const * pointLight
        ) const;

        void performOcclusionQueryPass(RT::CommandRecordState & recordState);

        void renderForOcclusionQueryPass(RT::CommandRecordState const & recordState);

        void performDisplayPass(RT::CommandRecordState & recordState) const;

        void renderForDisplayPass(RT::CommandRecordState const & recordState, AS::AlphaMode alphaMode) const;

        std::shared_ptr<RT::SamplerGroup> mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
        std::shared_ptr<RT::GpuTexture> mErrorTexture = nullptr;
        std::shared_ptr<RT::BufferGroup> mErrorBuffer{};

        // =========== Graphic =========== //

        // Display pass
        std::shared_ptr<RT::PipelineGroup> mDisplayPassPipeline{};

        // PointLight shadow pass
        std::shared_ptr<RT::PipelineGroup> mPointLightShadowPipeline {};
        std::unique_ptr<PointLightShadowRenderPass> mPointLightShadowRenderPass;
        std::unique_ptr<PointLightShadowResources> mPointLightShadowResources {};

        // Directional light shadow pass
        std::shared_ptr<RT::PipelineGroup> mDirectionalLightShadowPipeline {};
        std::unique_ptr<DirectionalLightShadowRenderPass> mDirectionalLightShadowRenderPass;
        std::unique_ptr<DirectionalLightShadowResource> mDirectionalLightShadowResources;

        // Depth pass pipeline
        std::shared_ptr<RT::PipelineGroup> mDepthPassPipeline{};
        std::unique_ptr<DepthPrePass> mDepthPrePass;

        // =========== Occlusion =========== //

        std::shared_ptr<RT::PipelineGroup> mOcclusionQueryPipeline{};
        std::unique_ptr<OcclusionRenderPass> mOcclusionRenderPass;

        struct OcclusionQueryData
        {
            std::vector<std::weak_ptr<VariantBase>> Variants {};
            std::vector<uint64_t> Results{};
            VkQueryPool Pool{};
        };
        std::vector<OcclusionQueryData> mOcclusionQueryDataList{}; // We need one per frame

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mGfxPerFrameDescriptorSetLayout{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mGfxPerEssenceDescriptorSetLayout{};
        
        RT::DescriptorSetGroup mGfxPerFrameDescriptorSetGroup{};

        std::weak_ptr<DebugEssence> mOcclusionEssence {};

        // =========== Compute =========== //

        std::shared_ptr<RT::PipelineGroup> mSkinningPipeline{};

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mSkinningPerEssenceDescriptorSetLayout{};
        std::shared_ptr<RT::DescriptorSetLayoutGroup> mSkinningPerVariantDescriptorSetLayout{};
    };

};
