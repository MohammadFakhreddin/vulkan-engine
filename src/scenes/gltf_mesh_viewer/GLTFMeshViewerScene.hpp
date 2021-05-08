#pragma once

#include "engine/DrawableObject.hpp"
#include "engine/Scene.hpp"
#include "engine/RenderFrontend.hpp"
#include "engine/RenderBackend.hpp"
#include "engine/pipelines/pbr/PBRModelPipeline.hpp"
#include "engine/pipelines/point_light/PointLightPipeline.hpp"

class GLTFMeshViewerScene final : public MFA::Scene {
public:

    explicit GLTFMeshViewerScene() = default;

    void Init() override;

    void OnDraw(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void OnUI(uint32_t delta_time, MFA::RenderFrontend::DrawPass & draw_pass) override;

    void Shutdown() override;

    void OnResize() override;

private:

    static constexpr float LightScale = 100.0f;
    
    struct ModelRenderRequiredData {
        bool isLoaded = false;
        MFA::RenderFrontend::GpuModel gpuModel {};
        std::string displayName {};
        std::string address {};
        MFA::DrawableObjectId drawableObjectId {};
        struct {
            struct {
                float rotationEulerAngle[3] {};
                float scale = 1;
                float translate[3] {0, 0, -10.0f};
                float translateMin[3] {-100.0f, -100.0f, -100.0f};
                float translateMax[3] {100.0f, 100.0f, 100.0f};
            } model {};
            struct {
                float position [3] {};
                float color[3]{
                    (252.0f/256.0f) * LightScale,
                    (212.0f/256.0f) * LightScale,
                    (64.0f/256.0f) * LightScale
                };
                float translateMin[3] {-200.0f, -200.0f, -200.0f};
                float translateMax[3] {200.0f, 200.0f, 200.0f};
            } light {};
        } initialParams {.model {}, .light {}};
    };

    void createModel(ModelRenderRequiredData & renderRequiredData);

    void destroyModels();

    void updateProjectionBuffer();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;

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
    }; 

    MFA::PBRModelPipeline::ViewProjectionData mPbrViewProjectionData {};
    MFA::PointLightPipeline::ViewProjectionData mPointLightViewProjectionData {};

    float m_model_rotation[3] {45.0f, 45.0f, 45.0f};
    float m_model_scale = 1.0f;
    float m_model_position[3] {0.0f, 0.0f, -6.0f};

    MFA::PBRModelPipeline::LightViewBuffer mLightViewData {
        .lightPosition = {},
        .cameraPosition = {0.0f, 0.0f, 0.0f},
    };

    float mLightPosition[3] {0.0f, 0.0f, -2.0f};
    float mLightColor[3]{};

    std::vector<ModelRenderRequiredData> mModelsRenderData {};
    int32_t mSelectedModelIndex = 0;
    int32_t mPreviousModelSelectedIndex = -1;

    MFA::RenderFrontend::SamplerGroup mSamplerGroup {};

    MFA::PBRModelPipeline mPbrPipeline {};
    MFA::PointLightPipeline mPointLightPipeline {};

    MFA::RenderBackend::GpuTexture mErrorTexture {};

    float mModelTranslateMin[3] {-100.0f, -100.0f, -100.0f};
    float mModelTranslateMax[3] {100.0f, 100.0f, 100.0f};

    float mLightTranslateMin[3] {-200.0f, -200.0f, -200.0f};
    float mLightTranslateMax[3] {200.0f, 200.0f, 200.0f};

    bool mIsLightVisible = true;

    MFA::RF::GpuModel mGpuPointLight {};
    MFA::DrawableObjectId mPointLightObjectId = 0;

};
