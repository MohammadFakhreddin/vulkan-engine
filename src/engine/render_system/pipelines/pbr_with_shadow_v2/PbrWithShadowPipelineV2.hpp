#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"

#include "glm/glm.hpp"

// Optimize this file using https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy

namespace MFA {

class ShadowRenderPassV2;
class DrawableVariant;

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
    struct DisplayPassAllStagesPushConstants {
        float modeTransform[16];
        float inverseNodeTransform[16];
        int skinIndex;
        uint32_t primitiveIndex;      // Unique id
        int placeholder0[2];
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

    explicit PBRWithShadowPipelineV2();

    ~PBRWithShadowPipelineV2() override;

    PBRWithShadowPipelineV2 (PBRWithShadowPipelineV2 const &) noexcept = delete;
    PBRWithShadowPipelineV2 (PBRWithShadowPipelineV2 &&) noexcept = delete;
    PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 const &) noexcept = delete;
    PBRWithShadowPipelineV2 & operator = (PBRWithShadowPipelineV2 &&) noexcept = delete;

    void Init (
        RT::SamplerGroup * samplerGroup, 
        RT::GpuTexture * errorTexture,
        float projectionNear,
        float projectionFar
    );

    void Shutdown() override;

    void PreRender(RT::DrawPass & drawPass, float deltaTime) override;

    void Render(RT::DrawPass & drawPass, float deltaTime) override;

    void OnResize() override {}

    void UpdateCameraView(const float view[16]);

    void UpdateCameraProjection(const float projection[16]);

    void UpdateLightPosition(const float lightPosition[3]);

    void UpdateCameraPosition(const float cameraPosition[3]);

    void UpdateLightColor(const float lightColor[3]);

    void CreateDisplayPassDescriptorSets(DrawableVariant * variant);

    void CreateShadowPassDescriptorSets(DrawableVariant * variant);

    DrawableVariant * CreateDrawableVariant(char const * essenceName) override;

private:

    void createDisplayPassDescriptorSetLayout();

    void createShadowPassDescriptorSetLayout();

    void destroyDescriptorSetLayout() const;

    void createDisplayPassPipeline();

    void createShadowPassPipeline();

    void destroyPipeline();

    void createUniformBuffers();

    void destroyUniformBuffers();

    void updateDisplayLightBuffer(RT::DrawPass const & drawPass);

    void updateShadowLightBuffer(RT::DrawPass const & drawPass);

    void updateShadowViewProjectionBuffer(RT::DrawPass const & drawPass);
    
    void updateShadowViewProjectionData();
    
    void updateDisplayViewProjectionBuffer(RT::DrawPass const & drawPass);
    
    inline static constexpr float SHADOW_WIDTH = 1024;
    inline static constexpr float SHADOW_HEIGHT = 1024;

    bool mIsInitialized = false;

    RT::SamplerGroup * mSamplerGroup = nullptr; // TODO Each gltf subMesh has its own settings
    RT::GpuTexture * mErrorTexture = nullptr;
    RT::UniformBufferGroup mErrorBuffer {};
    
    VkDescriptorSetLayout mDisplayPassDescriptorSetLayout {};
    RT::PipelineGroup mDisplayPassPipeline {};
    RT::UniformBufferGroup mDisplayLightAndCameraBuffer {};
    RT::UniformBufferGroup mDisplayViewProjectionBuffer {};
    
    VkDescriptorSetLayout mShadowPassDescriptorSetLayout {};
    RT::PipelineGroup mShadowPassPipeline {};
    RT::UniformBufferGroup mShadowLightBuffer {};

    std::unique_ptr<ShadowRenderPassV2> mShadowRenderPass;

    RT::UniformBufferGroup mShadowViewProjectionBuffer {};
    
    float mLightPosition[3] {};
    float mCameraPosition[3] {};
    float mLightColor[3] {};

    float mProjectionNear = 0.0f;
    float mProjectionFar = 0.0f;
    float mProjectionFarToNearDistance = 0.0f;
    
    glm::mat4 mShadowProjection;

    ShadowViewProjectionData mShadowViewProjectionData {};
    
    float mDisplayProjection [16] {};
    
    float mDisplayView [16] {};
    
    uint32_t mDisplayViewProjectionNeedUpdate = 0;
    uint32_t mDisplayLightNeedUpdate = 0;
    uint32_t mShadowLightNeedUpdate = 0;
    uint32_t mShadowViewProjectionNeedUpdate = 0;

};

};
