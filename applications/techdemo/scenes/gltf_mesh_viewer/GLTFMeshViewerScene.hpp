#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/camera/ObserverCameraComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"

namespace MFA
{
    class DebugRendererPipeline;
    class ColorComponent;
}

class GLTFMeshViewerScene final : public MFA::Scene
{
public:

    explicit GLTFMeshViewerScene();

    ~GLTFMeshViewerScene() override = default;

    GLTFMeshViewerScene(GLTFMeshViewerScene const &) noexcept = delete;
    GLTFMeshViewerScene(GLTFMeshViewerScene &&) noexcept = delete;
    GLTFMeshViewerScene & operator = (GLTFMeshViewerScene const &) noexcept = delete;
    GLTFMeshViewerScene & operator = (GLTFMeshViewerScene &&) noexcept = delete;

    void Init() override;

    void Update(float deltaTimeInSec) override;

    void OnUI();

    void Shutdown() override;
    
    bool RequiresUpdate() override;

private:

    static constexpr float LightScale = 100.0f;

    struct ModelRenderRequiredData
    {
        bool isLoaded = false;
        //std::shared_ptr<MFA::RT::GpuModel> gpuModel{};
        std::string displayName{};
        std::string address{};
        MFA::Entity * entity = nullptr;
        std::weak_ptr<MFA::MeshRendererComponent> meshRendererComponent{};
        std::weak_ptr<MFA::TransformComponent> transformComponent{};
        struct InitialParams
        {
            struct Model
            {
                float rotationEulerAngle[3]{};
                float scale = 1;
                float translate[3]{ 0, 0, -10.0f };
                float translateMin[3]{ -100.0f, -100.0f, -100.0f };
                float translateMax[3]{ 100.0f, 100.0f, 100.0f };
            } model;
            struct Light
            {
                float position[3]{};
                float color[3]{
                    (252.0f / 256.0f) * LightScale,
                    (212.0f / 256.0f) * LightScale,
                    (64.0f / 256.0f) * LightScale
                };
                float translateMin[3]{ -200.0f, -200.0f, -200.0f };
                float translateMax[3]{ 200.0f, 200.0f, 200.0f };
            } light;
            struct Camera
            {
                float position[3]{};
                float eulerAngles[3]{};
            } camera;
            explicit InitialParams()
                : model({})
                , light({})
                , camera({})
            {}
        } initialParams{};
    };

    void createModel(ModelRenderRequiredData & renderRequiredData) const;

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;

    struct PrimitiveInfo
    {
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

    std::vector<ModelRenderRequiredData> mModelsRenderData{};
    int32_t mSelectedModelIndex = 0;
    int32_t mPreviousModelSelectedIndex = -1;

    float mModelTranslateMin[3]{ -100.0f, -100.0f, -100.0f };
    float mModelTranslateMax[3]{ 100.0f, 100.0f, 100.0f };

    float mLightTranslateMin[3]{ -200.0f, -200.0f, -200.0f };
    float mLightTranslateMax[3]{ 200.0f, 200.0f, 200.0f };

    std::weak_ptr<MFA::TransformComponent> mPointLightTransform{};
    std::weak_ptr<MFA::ColorComponent> mPointLightColor{};

    MFA::DebugRendererPipeline * mDebugRenderPipeline = nullptr;

#ifdef __DESKTOP__
    static constexpr float FOV = 80;
#elif defined(__ANDROID__) || defined(__IOS__)
    static constexpr float FOV = 40;    // TODO It should be only for standing orientation
#else
#error Os is not handled
#endif

    std::weak_ptr<MFA::ObserverCameraComponent> mCamera{};

    int mUIRegisterId = 0;

};
