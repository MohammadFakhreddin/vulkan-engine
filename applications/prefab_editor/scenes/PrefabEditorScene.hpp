#pragma once

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "tools/Prefab.hpp"

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

    void essencesWindow();

    void saveAndLoadWindow();

    bool loadSelectedAsset(std::string const & fileAddress);

    void destroyAsset(int assetIndex);

    void entityUI(MFA::Entity * entity);

    void componentUI(MFA::Component * component, MFA::Entity * entity);

    void prepareDependencyLists();

    void prepareCreateComponentInstructionMap();
    
    void bindEditorSignalToEntity(MFA::Entity * entity);

    MFA::Entity * createPrefabEntity(char const * name, MFA::Entity * parent, bool createTransform = true);

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
    static constexpr float FOV = 80;
    
    std::shared_ptr<MFA::RT::GpuTexture> mErrorTexture{};

    std::string mPrefabName {};
    MFA::Entity * mPrefabRootEntity = nullptr;

    std::shared_ptr<MFA::RT::SamplerGroup> mSampler{};

    MFA::PBRWithShadowPipelineV2 mPbrPipeline{ this };
    MFA::DebugRendererPipeline mDebugRenderPipeline{};

    int mUIRecordId = 0;

    struct Asset
    {
        std::string fileAddress {};
        std::string essenceName {};
        std::shared_ptr<MFA::RT::GpuModel> gpuModel {};
    };
    std::vector<Asset> mLoadedAssets {};

    std::vector<std::string> mAvailableComponents {"Invalid"};
    std::unordered_map<int, std::vector<int>> mComponentDependencies {};
    std::unordered_map<int, std::vector<int>> mComponentsDependents {};

    struct CreateComponentInstruction
    {
        int familyType = -1;
        std::function<std::shared_ptr<MFA::Component>(MFA::Entity * entity)> function {};
    };
    std::unordered_map<std::string, CreateComponentInstruction> mCreateComponentInstructionMap {};

    // Input variables
    std::string mInputTextEssenceName {};
    int mSelectedComponentIndex = 0;
    int mSelectedEssenceIndex = 0;
    int mSelectedPipeline = 0;
    std::string mInputChildEntityName {};

    MFA::Prefab mPrefab;

};