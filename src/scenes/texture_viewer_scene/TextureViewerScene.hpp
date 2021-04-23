#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"

class TextureViewerScene final : public MFA::Scene {
public:

    TextureViewerScene() = default;
    void Init() override;
    void Shutdown() override;
    void OnDraw(MFA::U32 deltaTime, MFA::RenderFrontend::DrawPass & drawPass) override;
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

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
    void createDrawPipeline(MFA::U8 gpuShaderCount, MFA::RenderBackend::GpuShader * gpuShaders);
    void createModel();
    void createDrawableObject();

    MFA::RenderFrontend::GpuModel mGpuModel {};

    VkDescriptorSetLayout_T * mDescriptorSetLayout = nullptr;

    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    MFA::DrawableObject mDrawableObject {};

    MFA::RenderFrontend::SamplerGroup mSamplerGroup {};

    float mModelRotation[3] {45.0f, 45.0f, 45.0f};
    float mModelScale = 1.0f;
    float mModelPosition[3] {0.0f, 0.0f, -6.0f};

    int mMipLevel = 0;
    int mTotalMipCount = 0;
};