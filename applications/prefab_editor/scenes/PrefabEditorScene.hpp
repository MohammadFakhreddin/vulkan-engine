#pragma once

#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/scene_manager/Scene.hpp"

class PrefabEditorScene final : public MFA::Scene
{
public:

    explicit PrefabEditorScene();

    ~PrefabEditorScene() override;

    PrefabEditorScene(PrefabEditorScene const &) noexcept = delete;
    PrefabEditorScene(PrefabEditorScene &&) noexcept = delete;
    PrefabEditorScene & operator = (PrefabEditorScene const &) noexcept = delete;
    PrefabEditorScene & operator = (PrefabEditorScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnPostRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void Shutdown() override;

    void OnResize() override;

private:

    void onUI() const;

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
    static constexpr float FOV = 80;

    MFA::RT::GpuTexture mErrorTexture{};

    MFA::RT::GpuModel mPrefabModel{};
    MFA::Entity * mPrefabEntity = nullptr;

    MFA::RT::SamplerGroup mSampler{};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline{ this };
    MFA::DebugRendererPipeline mDebugRenderPipeline{};

    MFA::RT::GpuModel mSphereModel {};
    MFA::RT::GpuModel mCubeModel {};

    int mUIRecordId = 0;

};
