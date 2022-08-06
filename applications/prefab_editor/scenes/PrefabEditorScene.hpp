#pragma once

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "tools/Prefab.hpp"

namespace MFA
{
    namespace AssetSystem
    {
        struct Model;
    }

    class PBRWithShadowPipelineV2;
    class DebugRendererPipeline;
}

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

    void Update(float deltaTimeInSec) override;

    void Shutdown() override;
    
private:

    void onUI();

    void essencesWindow();

    void saveAndLoadWindow();

    /*void addEssenceToPipeline(
        std::string const & nameId,
        std::shared_ptr<MFA::AssetSystem::Model> const & cpuModel
    );*/

    bool loadSelectedAsset(
        std::string const & fileAddress,
        MFA::BasePipeline * pipeline,
        std::string displayName = ""
    );

    void destroyAsset(int assetIndex);

    void entityUI(MFA::Entity * entity);

    void componentUI(MFA::Component * component, MFA::Entity * entity);

    void prepareDependencyLists();

    void prepareCreateComponentInstructionMap();
    
    void bindEditorSignalToEntity(MFA::Entity * entity);

    MFA::Entity * createPrefabEntity(char const * name, MFA::Entity * parent, bool createTransform = true);

    void savePrefab();

    void loadPrefab();

    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
    static constexpr float FOV = 80;
    
    std::string mPrefabName {};
    MFA::Entity * mPrefabRootEntity = nullptr;

    int mUIRecordId = 0;

    struct Asset
    {
        std::string nameId {};
        std::string displayName {};
    };
    std::vector<Asset> mLoadedAssets {};

    std::vector<std::string> mAvailableComponents {"Invalid"};
    // TODO: Each component should hold these information
    std::unordered_map<std::string, std::vector<std::string>> mComponentDependencies {};
    std::unordered_map<std::string, std::vector<std::string>> mComponentsDependents {};

    struct ComponentCreationInstruction
    {
        std::function<std::shared_ptr<MFA::Component>(MFA::Entity * entity)> function {};
    };
    std::unordered_map<std::string, ComponentCreationInstruction> mCreateComponentInstructionMap {};

    // Input variables
    std::string mInputTextEssenceName {};
    int mSelectedComponentIndex = 0;
    int mSelectedEssenceIndex = 0;
    int mSelectedPipeline = 0;
    std::string mInputChildEntityName {};

    MFA::Prefab mPrefab;

    std::vector<MFA::BasePipeline *> mAllPipelines {};

    MFA::PBRWithShadowPipelineV2 * mPBR_Pipeline = nullptr;
    MFA::DebugRendererPipeline * mDebugPipeline = nullptr;

};
