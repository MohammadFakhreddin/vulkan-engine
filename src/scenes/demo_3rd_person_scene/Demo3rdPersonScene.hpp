#pragma once

#include "engine/scene_manager/Scene.hpp"
#include "engine/camera/ThirdPersonCameraComponent.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"

class Demo3rdPersonScene final : public MFA::Scene {
public:

    explicit Demo3rdPersonScene();

    ~Demo3rdPersonScene() override;

    Demo3rdPersonScene (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene (Demo3rdPersonScene &&) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::CommandRecordState & drawPass) override;

    void OnRender(float deltaTimeInSec, MFA::RT::CommandRecordState & drawPass) override;

    void OnPostRender(float deltaTimeInSec, MFA::RT::CommandRecordState & drawPass) override;

    void Shutdown() override;

    void OnResize() override;

private:

    void onUI() const;

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
    
    MFA::TransformComponent * mPlayerTransform = nullptr;
    MFA::MeshRendererComponent * mPlayerMeshRenderer = nullptr;

    MFA::RT::GpuModel mMapModel {};

    MFA::ThirdPersonCameraComponent * mThirdPersonCamera = nullptr;

    MFA::RT::SamplerGroup mSampler {};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline {};
    MFA::DebugRendererPipeline mDebugRenderPipeline {};
    
    float mLightPosition[3] {1.0f, 0.0f, -3.0f};

    static constexpr float LightScale = 100.0f;
    float mLightColor[3] {
        (252.0f/256.0f) * LightScale,
        (212.0f/256.0f) * LightScale,
        (64.0f/256.0f) * LightScale
    };

    MFA::RT::GpuModel mSphereModel {};
    MFA::RT::GpuModel mCubeModel {};
    
    int mUIRecordId = 0;

};
