#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/render_passes/OffScreenRenderPassV2.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class PBRWithShadowPipelineV2 final : public BasePipeline {
public:

    struct PrimitiveInfo {
        alignas(16) float baseColorFactor[4];
        float emissiveFactor[3];
        int placeholder0;
        alignas(4) int hasBaseColorTexture;
        alignas(4) float metallicFactor;
        alignas(4) float roughnessFactor;
        alignas(4) int hasMixedMetallicRoughnessOcclusionTexture;
        alignas(4) int hasNormalTexture;
        alignas(4) int hasEmissiveTexture;
        alignas(4) int hasSkin;
    };

    // TODO We could have a model buffer for each drawable but with a single viewProjection buffer
    struct ModelData {   // For vertices in Vertex shader
        alignas(64) float model[16];
    };

    struct DisplayViewData {
        alignas(64) float view[16];
    };

    struct DisplayProjectionData {
        alignas(64) float projection[16];
    };

    struct ShadowViewProjectionData {
        float viewMatrices[6][16];
    };

    struct ShadowPushConstants {
        int faceIndex;
    };

    struct DisplayLightAndCameraData {
        alignas(16) float lightPosition[3];
        alignas(16) float cameraPosition[3];
        alignas(16) float lightColor[3];
        alignas(4) float projectFarToNearDistance;
    };

    struct ShadowLightData {
        alignas(16) float lightPosition[4];
        alignas(4) float projectionMatrixDistance;
    };

    explicit PBRWithShadowPipelineV2() = default;

    ~PBRWithShadowPipelineV2() override {
        if (mIsInitialized) {
            Shutdown();
        }
    }

    PBRWithShadowPipelineV2 (PBRWithShadowPipelineV2 const &) noexcept = delete;
    PBRWithShadowPipelineV2 (PBRWithShadowPipelineV2 &&) noexcept = delete;
    PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 const &) noexcept = delete;
    PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 &&) noexcept = delete;

    void Init(
        RF::SamplerGroup * samplerGroup, 
        RB::GpuTexture * errorTexture,
        float projectionNear,
        float projectionFar
    );

    void Shutdown();

    void Update(RF::DrawPass & drawPass, float deltaTime, uint32_t idsCount, DrawableObjectId * ids) override;

    void Render(RF::DrawPass & drawPass, float deltaTime, uint32_t idsCount, DrawableObjectId * ids) override;

    void OnResize() override {}

    DrawableObjectId AddGpuModel(RF::GpuModel & gpuModel) override;

    bool RemoveGpuModel(DrawableObjectId drawableObjectId) override;

    bool UpdateModel(
        DrawableObjectId drawableObjectId, 
        ModelData const & modelData
    );

    bool UpdateView(
        DrawableObjectId drawableObjectId, 
        DisplayViewData const & viewData
    );

    bool UpdateProjection(
        DrawableObjectId drawableObjectId,
        DisplayProjectionData const & projectionData
    );

    void UpdateLightPosition(float lightPosition[3]);

    void UpdateCameraPosition(float cameraPosition[3]);

    void UpdateLightColor(float lightColor[3]);

    DrawableObject * GetDrawableById(DrawableObjectId objectId);

    void CreateDisplayPassDescriptorSets(DrawableObject * drawableObject);

    void CreateShadowPassDescriptorSets(DrawableObject * drawableObject);

private:

    void createDisplayPassDescriptorSetLayout();

    void createShadowPassDescriptorSetLayout();

    void destroyDescriptorSetLayout() const;

    void createDisplayPassPipeline();

    void createShadowPassPipeline();

    void destroyPipeline();

    void createUniformBuffers();

    void destroyUniformBuffers();

    inline static constexpr float SHADOW_WIDTH = 1024;
    inline static constexpr float SHADOW_HEIGHT = 1024;

    bool mIsInitialized = false;

    RF::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
    RB::GpuTexture * mErrorTexture = nullptr;
    RF::UniformBufferGroup mErrorBuffer {};
    
    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};
    
    VkDescriptorSetLayout mDisplayPassDescriptorSetLayout {};
    RF::DrawPipeline mDisplayPassPipeline {};
    RF::UniformBufferGroup mDisplayLightViewBuffer {};
    RF::UniformBufferGroup mDisplayViewBuffer {};
    RF::UniformBufferGroup mDisplayProjectionBuffer {};

    VkDescriptorSetLayout mShadowPassDescriptorSetLayout {};
    RF::DrawPipeline mShadowPassPipeline {};
    RF::UniformBufferGroup mShadowLightBuffer {};
    OffScreenRenderPassV2 mShadowRenderPass {
        static_cast<uint32_t>(SHADOW_WIDTH),
        static_cast<uint32_t>(SHADOW_HEIGHT)
    };
    RF::UniformBufferGroup mShadowViewProjectionBuffer {};
    RF::UniformBufferGroup mShadowProjectionBuffer {};

    float mLightPosition[3] {};
    float mCameraPosition[3] {};
    float mLightColor[3] {};

    float mProjectionNear = 0.0f;
    float mProjectionFar = 0.0f;
    float mProjectionFarToNearDistance = 0.0f;

    bool mNeedToUpdateDisplayLightBuffer = true;
    bool mNeedToUpdateShadowLightBuffer = true;
    
    DrawableObjectId mNextDrawableObjectId = 0;

    glm::mat4 mShadowProjection {};

    ShadowMatricesData mViewMatricesData {};


};

};
