#pragma once

#include "engine/Scene.hpp"
#include "engine/camera/ThirdPersonCamera.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/point_light/PointLightPipeline.hpp"

class Demo3rdPersonScene final : public MFA::Scene {
public:

    explicit Demo3rdPersonScene();

    ~Demo3rdPersonScene() override;

    Demo3rdPersonScene (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene (Demo3rdPersonScene &&) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnPostRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void Shutdown() override;

    void OnResize() override;

private:

    void updateProjectionBuffer();

    void OnUI();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
#ifdef __DESKTOP__
    static constexpr float FOV = 80;
#elif defined(__ANDROID__) || defined(__IOS__)
    static constexpr float FOV = 40;    // TODO It should be only for standing orientation
#else
#error Os is not handled
#endif

    MFA::RT::GpuTexture mErrorTexture {};

    MFA::RT::GpuModel mSoldierGpuModel {};
    MFA::DrawableVariant * mSoldierVariant = nullptr;
    std::vector<MFA::DrawableVariant *> mNPCs {};

    MFA::RT::GpuModel mMapModel {};
    MFA::DrawableVariant * mMapVariant = nullptr;

    MFA::ThirdPersonCamera mCamera {
        FOV,
        Z_FAR,
        Z_NEAR,
    };

    MFA::RT::SamplerGroup mSampler {};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline {};
    MFA::PointLightPipeline mPointLightPipeline {};

    MFA::UIRecordObject mRecordObject;

    float mLightPosition[3] {1.0f, 0.0f, -3.0f};

    static constexpr float LightScale = 100.0f;
    float mLightColor[3] {
        (252.0f/256.0f) * LightScale,
        (212.0f/256.0f) * LightScale,
        (64.0f/256.0f) * LightScale
    };

    MFA::RT::GpuModel mPointLightModel {};
    MFA::DrawableVariant * mPointLightVariant = nullptr;

};
