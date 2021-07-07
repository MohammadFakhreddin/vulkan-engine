#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/Scene.hpp"

class TextureViewerScene final : public MFA::Scene {
public:

    TextureViewerScene();
    void Init() override;
    void Shutdown() override;
    void OnDraw(float deltaTimeInSec, MFA::RenderFrontend::DrawPass & drawPass) override;
    void OnUI();
    void OnResize() override;

private:

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;

    struct ViewProjectionBuffer {   // For vertices in Vertex shader
        alignas(64) float view[16];
        alignas(64) float perspective[16];
    } mViewProjectionBuffer {};

    struct ImageOptionsBuffer {
        float mipLevel;
        // int sliceIndex; // TODO: Consider adding sliceIndex in future
    } mImageOptionsBuffer {};

    void createDescriptorSetLayout();
    void createDrawPipeline(uint8_t gpuShaderCount, MFA::RenderBackend::GpuShader * gpuShaders);
    void createModel();
    void createDrawableObject();
    void updateProjection();

    MFA::RenderFrontend::GpuModel mGpuModel {};

    VkDescriptorSetLayout mDescriptorSetLayout {};

    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    std::unique_ptr<MFA::DrawableObject> mDrawableObject = nullptr;

    MFA::RenderFrontend::SamplerGroup mSamplerGroup {};

    float mModelRotation[3] {0.0f, 0.0f, 0.0f};
    float mModelScale = 1.0f;
    float mModelPosition[3] {0.0f, 0.0f, -3.0f};

    int mMipLevel = 0;
    int mTotalMipCount = 0;

    MFA::UIRecordObject mRecordObject;
};