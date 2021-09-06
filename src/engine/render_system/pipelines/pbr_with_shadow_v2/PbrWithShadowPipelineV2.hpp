#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class ShadowRenderPassV2;

class PBRWithShadowPipelineV2 final : public BasePipeline {
public:

    struct DisplayViewProjectionData {
        alignas(64) float projection[16];
    };
    static_assert(sizeof(DisplayViewProjectionData) == 64);

    struct ShadowViewProjectionData {
        float viewMatrices[6][16];
    };

    struct ShadowPassVertexStagePushConstants {
        float modeTransform[16];
        float inverseNodeTransform[16];
        int faceIndex;
        int skinIndex;
        int placeholder0[2];
    };
    //static_assert(sizeof(ShadowPassVertexStagePushConstants) == 136);
    struct DisplayPassAllStagesPushConstants {
        float modeTransform[16];
        float inverseNodeTransform[16];
        int skinIndex;
        uint32_t primitiveIndex;      // Unique id
        int placeholder0[2];
    };
    //static_assert(sizeof(DisplayPassAllStagesPushConstants) == 136);

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

    explicit PBRWithShadowPipelineV2();

    ~PBRWithShadowPipelineV2() override;

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

    void PreRender(RF::DrawPass & drawPass, float deltaTime, uint32_t idsCount, DrawableObjectId * ids) override;

    void Render(RF::DrawPass & drawPass, float deltaTime, uint32_t idsCount, DrawableObjectId * ids) override;

    void PostRender(RF::DrawPass & drawPass, float deltaTime, uint32_t idsCount, DrawableObjectId * ids) override;

    void OnResize() override {}

    DrawableObjectId AddGpuModel(RF::GpuModel & gpuModel) override;

    bool RemoveGpuModel(DrawableObjectId drawableObjectId) override;

    bool UpdateModel(
        DrawableObjectId drawableObjectId, 
        float modelTransform[16]
    );

    void UpdateCameraView(float view[16]);

    void UpdateCameraProjection(float projection[16]);

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

    void updateDisplayLightBuffer(RF::DrawPass const & drawPass);

    void updateShadowLightBuffer(RF::DrawPass const & drawPass);

    void updateShadowViewProjectionBuffer(RF::DrawPass const & drawPass);
    
    void updateShadowViewProjectionData();
    
    void updateDisplayViewProjectionBuffer(RF::DrawPass const & drawPass);
    
    inline static constexpr float SHADOW_WIDTH = 1024;
    inline static constexpr float SHADOW_HEIGHT = 1024;

    bool mIsInitialized = false;

    RF::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
    RB::GpuTexture * mErrorTexture = nullptr;
    RF::UniformBufferGroup mErrorBuffer {};
    
    std::unordered_map<DrawableObjectId, std::unique_ptr<DrawableObject>> mDrawableObjects {};
    
    VkDescriptorSetLayout mDisplayPassDescriptorSetLayout {};
    RF::DrawPipeline mDisplayPassPipeline {};
    RF::UniformBufferGroup mDisplayLightAndCameraBuffer {};
    RF::UniformBufferGroup mDisplayViewProjectionBuffer {};
    
    VkDescriptorSetLayout mShadowPassDescriptorSetLayout {};
    RF::DrawPipeline mShadowPassPipeline {};
    RF::UniformBufferGroup mShadowLightBuffer {};

    std::unique_ptr<ShadowRenderPassV2> mShadowRenderPass;

    RF::UniformBufferGroup mShadowViewProjectionBuffer {};
    
    float mLightPosition[3] {};
    float mCameraPosition[3] {};
    float mLightColor[3] {};

    float mProjectionNear = 0.0f;
    float mProjectionFar = 0.0f;
    float mProjectionFarToNearDistance = 0.0f;

    DrawableObjectId mNextDrawableObjectId = 0;

    glm::mat4 mShadowProjection {};

    ShadowViewProjectionData mShadowViewProjectionData {};
    
    float mDisplayProjection [16] {};
    
    float mDisplayView [16] {};
    
    uint32_t mDisplayViewProjectionNeedUpdate = 0;
    uint32_t mDisplayLightNeedUpdate = 0;
    uint32_t mShadowLightNeedUpdate = 0;
    uint32_t mShadowViewProjectionNeedUpdate = 0;

};

};
