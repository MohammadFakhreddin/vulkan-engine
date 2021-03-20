#pragma once


#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/RenderBackend.hpp"

class GLTFMeshViewerScene final : public MFA::Scene {
public:

    explicit GLTFMeshViewerScene() = default;

    void Init() override;

    void OnDraw(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void OnUI(MFA::U32 delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void Shutdown() override;

private:

    struct ModelRenderRequiredData {
        bool isLoaded = false;
        MFA::RenderFrontend::GpuModel gpuModel {};
        std::string displayName {};
        std::string address {};
        MFA::DrawableObject drawableObject {};
    };

    void createModel(ModelRenderRequiredData & renderRequiredData);

    void destroyModels();

    void createDrawPipeline(MFA::U8 gpu_shader_count, MFA::RenderBackend::GpuShader * gpu_shaders);

    void createDescriptorSetLayout();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;

    struct SubMeshInfo {
        alignas(16) float baseColorFactor[4];
        float emissiveFactor[3];
        int placeholder0;
        alignas(4) int hasBaseColorTexture;
        alignas(4) float metallicFactor;
        alignas(4) float roughnessFactor;
        alignas(4) int hasMetallicRoughnessTexture;
        alignas(4) int hasNormalTexture;
        alignas(4) int hasEmissiveFactor;
    }; 

    struct ModelTransformBuffer {   // For vertices in Vertex shader
        float rotation[16];
        float transformation[16];
        float perspective[16];
    } m_translate_data {};

    float m_model_rotation[3] {45.0f, 45.0f, 45.0f};
    float m_model_position[3] {0.0f, 0.0f, -6.0f};

    struct LightViewBuffer {
        alignas(16) float light_position[3];
        alignas(16) float camera_position[3];
        alignas(16) float light_color[3];
    } m_lv_data {
        .light_position = {},
        .camera_position = {0.0f, 0.0f, 0.0f},
    };

    float m_light_position[3] {0.0f, 0.0f, -2.0f};
    float m_light_color[3] {252.0f/256.0f, 212.0f/256.0f, 64.0f/256.0f};

    std::vector<ModelRenderRequiredData> mModelsRenderData {};
    MFA::I32 mSelectedModelIndex = 0;

    MFA::RenderFrontend::SamplerGroup m_sampler_group {};

    VkDescriptorSetLayout_T * m_descriptor_set_layout = nullptr;

    MFA::RenderFrontend::DrawPipeline m_draw_pipeline {};

    MFA::RenderFrontend::UniformBufferGroup m_lv_buffer {};

    MFA::RenderBackend::GpuTexture m_error_texture {};

};