#pragma once

#include "engine/scene_manager/Scene.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockPlatforms.hpp"
#include "engine/camera/ThirdPersonCameraComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "tools/Prefab.hpp"

class Demo3rdPersonScene final : public MFA::Scene {
public:

    explicit Demo3rdPersonScene();

    ~Demo3rdPersonScene() override;

    Demo3rdPersonScene (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene (Demo3rdPersonScene &&) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnPostRender(float deltaTimeInSec) override;

    void Shutdown() override;

    bool useDisplayPassDepthImageAsUndefined() override;

private:

    void onUI();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
#ifdef __DESKTOP__
    static constexpr float FOV = 80;
#elif defined(__ANDROID__) || defined(__IOS__)
    static constexpr float FOV = 40;    // TODO It should be only for standing orientation
#else
#error Os is not handled
#endif

    std::shared_ptr<MFA::RT::GpuTexture> mErrorTexture {};

    //std::shared_ptr<MFA::RT::GpuModel> mSoldierGpuModel {};
    
    std::weak_ptr<MFA::TransformComponent> mPlayerTransform {};
    std::weak_ptr<MFA::MeshRendererComponent> mPlayerMeshRenderer {};

    std::shared_ptr<MFA::RT::GpuModel> mMapModel {};

    std::weak_ptr<MFA::ThirdPersonCameraComponent> mThirdPersonCamera {};

    std::shared_ptr<MFA::RT::SamplerGroup> mSampler {};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline {this};
    MFA::DebugRendererPipeline mDebugRenderPipeline {};

    int mUIRecordId = 0;

    MFA::Prefab mSoldierPrefab;
    MFA::Prefab mSponzaPrefab;

    bool mEnableDebugPipeline = false;

};
