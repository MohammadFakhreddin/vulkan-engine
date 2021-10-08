#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/camera/ObserverCameraComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"

namespace MFA
{
    class ColorComponent;
}

class GLTFMeshViewerScene final : public MFA::Scene {
public:

    explicit GLTFMeshViewerScene();

    ~GLTFMeshViewerScene() override = default;
   
    GLTFMeshViewerScene (GLTFMeshViewerScene const &) noexcept = delete;
    GLTFMeshViewerScene (GLTFMeshViewerScene &&) noexcept = delete;
    GLTFMeshViewerScene & operator = (GLTFMeshViewerScene const &) noexcept = delete;
    GLTFMeshViewerScene & operator = (GLTFMeshViewerScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnPostRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnUI();

    void Shutdown() override;

    void OnResize() override;

private:

    static constexpr float LightScale = 100.0f;
    
    struct ModelRenderRequiredData {
        bool isLoaded = false;
        MFA::RT::GpuModel gpuModel {};
        std::string displayName {};
        std::string address {};
        MFA::Entity * entity = nullptr;
        MFA::MeshRendererComponent * meshRendererComponent = nullptr;
        MFA::TransformComponent * transformComponent = nullptr;
        struct InitialParams {
            struct Model {
                float rotationEulerAngle[3] {};
                float scale = 1;
                float translate[3] {0, 0, -10.0f};
                float translateMin[3] {-100.0f, -100.0f, -100.0f};
                float translateMax[3] {100.0f, 100.0f, 100.0f};
            } model;
            struct Light {
                float position [3] {};
                float color[3]{
                    (252.0f/256.0f) * LightScale,
                    (212.0f/256.0f) * LightScale,
                    (64.0f/256.0f) * LightScale
                };
                float translateMin[3] {-200.0f, -200.0f, -200.0f};
                float translateMax[3] {200.0f, 200.0f, 200.0f};
            } light;
            struct Camera {
                float position [3] {};
                float eulerAngles [3] {};
            } camera;
            explicit InitialParams()
                : model({})
                , light({})
                , camera({})
            {}
        } initialParams {};
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

    float mModelRotation[3] {45.0f, 45.0f, 45.0f};
    float mModelScale = 1.0f;
    float mModelPosition[3] {0.0f, 0.0f, -6.0f};

    float mLightPosition[3] {0.0f, 0.0f, -2.0f};
    float mLightColor[3] {};

    std::vector<ModelRenderRequiredData> mModelsRenderData {};
    int32_t mSelectedModelIndex = 2;
    int32_t mPreviousModelSelectedIndex = -1;

    MFA::RT::SamplerGroup mSamplerGroup {};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline {};
    MFA::DebugRendererPipeline mPointLightPipeline {};

    MFA::RT::GpuTexture mErrorTexture {};

    float mModelTranslateMin[3] {-100.0f, -100.0f, -100.0f};
    float mModelTranslateMax[3] {100.0f, 100.0f, 100.0f};

    float mLightTranslateMin[3] {-200.0f, -200.0f, -200.0f};
    float mLightTranslateMax[3] {200.0f, 200.0f, 200.0f};

    bool mIsLightVisible = true;

    MFA::RT::GpuModel mPointLightModel {};

    MFA::TransformComponent * mPointLightTransform = nullptr;
    MFA::ColorComponent * mPointLightColor = nullptr;
    
    bool mIsObjectViewerWindowVisible = false;
    bool mIsLightWindowVisible = false;
    bool mIsCameraWindowVisible = false;
    bool mIsDrawableVariantWindowVisible = false;

#ifdef __DESKTOP__
    static constexpr float FOV = 80;
#elif defined(__ANDROID__) || defined(__IOS__)
    static constexpr float FOV = 40;    // TODO It should be only for standing orientation
#else
#error Os is not handled
#endif

    MFA::ObserverCameraComponent * mCamera = nullptr;

    int mUIRegisterId = 0;

};
