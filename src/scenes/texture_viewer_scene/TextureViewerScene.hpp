#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"

class TextureViewerScene final : public MFA::Scene {
public:

    TextureViewerScene() = default;
    void Init() override;
    void Shutdown() override;
    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;
    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

private:

    void createDescriptorSetLayout();
    void createDrawPipeline(MFA::U8 gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders);

    MFA::RenderBackend::GpuTexture mGpuTexture;
    VkDescriptorSetLayout_T * mDescriptorSetLayout = nullptr;
    MFA::RenderFrontend::DrawPipeline mDrawPipeline {};

    MFA::DrawableObject mDrawableObject {};

    MFA::RenderFrontend::SamplerGroup mSamplerGroup {};
};