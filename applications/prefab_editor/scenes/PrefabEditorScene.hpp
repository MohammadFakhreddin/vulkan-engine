#pragma once

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/Component.hpp"
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

    void onUI();

    bool loadSelectedAsset(std::string const & fileAddress);

    void destroyPrefabModelAndEssence();

    void entityUI(MFA::Entity * entity);

    void componentUI(MFA::Component * component, MFA::Entity * entity);

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
    static constexpr float FOV = 80;
    static constexpr char const * PREFAB_ESSENCE = "PrefabEssence";

    MFA::RT::GpuTexture mErrorTexture{};

    std::string mPrefabName = "";
    MFA::RT::GpuModel mPrefabGpuModel {};
    MFA::Entity * mPrefabEntity = nullptr;

    MFA::RT::SamplerGroup mSampler{};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline{ this };
    MFA::DebugRendererPipeline mDebugRenderPipeline{};

    MFA::RT::GpuModel mSphereModel {};
    MFA::RT::GpuModel mCubeModel {};

    int mUIRecordId = 0;

    int mSelectedComponentIndex = 0;
    std::vector<std::string> mAvailableComponents {
        "Invalid",
        "TransformComponent",
        "MeshRendererComponent",
        "BoundingVolumeRendererComponent",
        "SphereBoundingVolumeComponent",
        "AxisAlignedBoundingBoxes",
        "ColorComponent",
        "ObserverCameraComponent",
        "ThirdPersonCamera",
        "PointLightComponent",
        "DirectionalLightComponent"
    };

};
