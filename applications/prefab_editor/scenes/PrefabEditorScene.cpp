#include "PrefabEditorScene.hpp"

#include "WindowsApi.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/camera/ObserverCameraComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "tools/PrefabFileStorage.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"
#include "engine/asset_system/AssetDebugMesh.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::PrefabEditorScene()
    : mPrefab(EntitySystem::CreateEntity("Prefab", nullptr))
{};

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::~PrefabEditorScene() = default;

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::Init()
{
    Scene::Init();

    prepareDependencyLists();
    prepareCreateComponentInstructionMap();

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    {// Creating observer camera and directional light
        auto * entity = EntitySystem::CreateEntity("CameraEntity", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const observerCamera = entity->AddComponent<ObserverCameraComponent>(FOV, Z_NEAR, Z_FAR);
        MFA_ASSERT(observerCamera != nullptr);
        SetActiveCamera(observerCamera);

        entity->AddComponent<DirectionalLightComponent>();

        auto const colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent != nullptr);
        float const lightScale = 5.0f;
        float lightColor[3]{
            1.0f * lightScale,
            1.0f * lightScale,
            1.0f * lightScale
        };
        colorComponent->SetColor(lightColor);

        auto const transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->SetLocalRotation(glm::vec3(90.0f, 0.0f, 0.0f));

        entity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);
        EntitySystem::InitEntity(entity);
    }

    mPrefabRootEntity = createPrefabEntity("PrefabRoot", GetRootEntity());

    mPrefab.AssignPreBuiltEntity(mPrefabRootEntity);

    mAllPipelines = SceneManager::GetAllPipelines();

    mPBR_Pipeline = SceneManager::GetPipeline<PBRWithShadowPipelineV2>();
    MFA_ASSERT(mPBR_Pipeline != nullptr);

    mDebugPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
    MFA_ASSERT(mDebugPipeline != nullptr);

    loadSelectedAsset("Sphere", mDebugPipeline);
    loadSelectedAsset("CubeStrip", mDebugPipeline);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::Update(float const deltaTimeInSec)
{}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::Shutdown()
{
    Scene::Shutdown();
    UI::UnRegister(mUIRecordId);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::onUI()
{
    saveAndLoadWindow();
    essencesWindow();
    EntitySystem::OnUI();
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::essencesWindow()
{
    UI::BeginWindow("Essences");
    if (UI::TreeNode("Available essences"))
    {
        for (auto const & asset : mLoadedAssets)
        {
            if (UI::TreeNode(asset.displayName.c_str()))
            {
                UI::Text(asset.nameId.c_str());
                UI::TreePop();
            }
        }
        UI::TreePop();
    }
    if (UI::TreeNode("Create new essence"))
    {
        UI::InputText("Essence name", mInputTextEssenceName);
        UI::Button("Create", [this]()->void
        {
            std::vector<WinApi::Extension> const extensions{
                WinApi::Extension {
                    .name = "Gltf files",
                    .value = "*.gltf"
                },
                WinApi::Extension {
                    .name = "Glb files",
                    .value = "*.glb"
                }
            };

            // File picker windows (TODO: Multi-platform)
            std::string fileAddress;
            if (WinApi::TryToPickFile(extensions, fileAddress) == false)
            {
                MFA_LOG_WARN("No valid file address picked!");
                return;
            }

            // Creating relative address
            std::string relativeAddress;
            bool const success = Path::RelativeToAssetFolder(
                relativeAddress,
                relativeAddress
            );
            if (success == false)
            {
                MFA_LOG_WARN("All assets and prefabs must be placed in asset folder for portability\n");
                return;
            }

            if (loadSelectedAsset(relativeAddress, mPBR_Pipeline, mInputTextEssenceName) == false)
            {
                MFA_LOG_WARN("Loading asset failed");
            }

        });
        UI::TreePop();
    }
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::saveAndLoadWindow()
{
    UI::BeginWindow("Save and load panel");
    UI::Button("Save", [this]()->void
    {
        savePrefab();
    });
    UI::Button("Load", [this]()->void
    {
        loadPrefab();
    });

    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

bool PrefabEditorScene::loadSelectedAsset(
    std::string const & fileAddress,
    MFA::BaseMaterial * pipeline,
    std::string displayName
)
{
    MFA_LOG_INFO("Trying to load file with address %s", fileAddress.c_str());
    MFA_ASSERT(fileAddress.empty() == false);

    if (displayName.empty())
    {
        displayName = fileAddress;
    }

    RC::AcquireEssence(fileAddress, pipeline, [this, fileAddress, displayName](bool const success)->void{
        if (success == false)
        {
            MFA_LOG_WARN("Failed to load %s", fileAddress.c_str());
            return;
        }

        for (auto & asset : mLoadedAssets)
        {
            if (asset.nameId == fileAddress)
            {
                return;
            }
        }

        MFA_LOG_WARN("Loading file is complete.");

        mLoadedAssets.emplace_back(Asset{
            .nameId = fileAddress,
            .displayName = displayName
        });
    });
    //auto const cpuModel = RC::AcquireCpuModel(fileAddress);
    //if (cpuModel == nullptr)
    //{
    //    MFA_LOG_WARN("Failed to load selected asset from file");
    //    return false;
    //}


    /*if (mPBR_Pipeline->essenceExists(gpuModel->nameOrAddress) == false)
    {
        auto * mesh = cpuModel->mesh.get();

        MFA_ASSERT(mesh != nullptr);

        auto * pbrMesh = dynamic_cast<AS::PBR::Mesh *>(mesh);
        if (pbrMesh != nullptr)
        {
            mPBR_Pipeline->addEssence(std::make_shared<PBR_Essence>(gpuModel, pbrMesh->getMeshData()));
        } else {

        }
    }*/
    //addEssenceToPipeline(fileAddress, cpuModel);

    mInputTextEssenceName = "";

    return true;
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::destroyAsset(int const assetIndex)
{
    auto const & asset = mLoadedAssets[assetIndex];
    mPBR_Pipeline->destroyEssence(asset.nameId);
    mLoadedAssets.erase(mLoadedAssets.begin() + assetIndex);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::entityUI(Entity * entity)
{
    MFA_ASSERT(entity != nullptr);
    if (UI::TreeNode("New component"))
    {

        UI::Combo(
            "Component name",
            &mSelectedComponentIndex,
            mAvailableComponents
        );

        switch (mSelectedComponentIndex)
        {
        case 2: // MeshRendererComponent
        {
            std::vector<std::string> pipelineNames{};
            for (auto const * pipeline : mAllPipelines)
            {
                pipelineNames.emplace_back(pipeline->GetName());
            }
            UI::Combo(
                "Pipeline",
                &mSelectedPipeline,
                pipelineNames
            );

            std::vector<std::string> essenceNames{};
            for (auto & asset : mLoadedAssets)
            {
                essenceNames.emplace_back(asset.displayName);
            }

            UI::Combo(
                "Essence",
                &mSelectedEssenceIndex,
                essenceNames
            );

        }
        break;
        default:
            break;
        }
        UI::Button("Add component", [this, entity]()->void
        {
            if (mSelectedComponentIndex <= 0)
            {
                return;
            }
            MFA_DEFER{
                mSelectedComponentIndex = 0;
            };

            auto const newComponentName = mAvailableComponents[mSelectedComponentIndex];

            auto const checkForDependencies = [this, entity, &newComponentName](std::string const componentName)->bool
            {
                auto const findResult = mComponentDependencies.find(componentName);
                MFA_ASSERT(findResult != mComponentDependencies.end());
                for (auto const & dependencyName : findResult->second)
                {
                    auto dependencyComponent = entity->GetComponent(dependencyName);
                    if (dependencyComponent == nullptr)
                    {
                        MFA_LOG_WARN(
                            "Cannot create component with name %s because a dependancy does not exists"
                            , newComponentName.c_str()
                        );
                        return false;
                    }
                }
                return true;
            };

            std::shared_ptr<Component> newComponent{};

            auto const findCreateInstructionResult = mCreateComponentInstructionMap.find(newComponentName);
            if (findCreateInstructionResult != mCreateComponentInstructionMap.end())
            {
                if (checkForDependencies(findCreateInstructionResult->first))
                {
                    newComponent = findCreateInstructionResult->second.function(entity);
                }
            }

            if (newComponent == nullptr)
            {
                return;
            }
            newComponent->Init();
            EntitySystem::UpdateEntity(entity);

            newComponent->EditorSignal.Register(this, &PrefabEditorScene::componentUI);
        });
        if (UI::TreeNode("Add child"))
        {
            UI::InputText("ChildName", mInputChildEntityName);
            UI::Button("Add", [entity, this]()->void
            {
                createPrefabEntity(mInputChildEntityName.c_str(), entity);
                mInputChildEntityName = "";
            });
            UI::TreePop();
        }
        if (UI::TreeNode("Remove entity"))
        {
            UI::Button("Remove", [entity, this]()->void
            {
                EntitySystem::DestroyEntity(entity);
            });
            UI::TreePop();
        }
        UI::TreePop();
    }
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::componentUI(Component * component, Entity * entity)
{
    MFA_ASSERT(component != nullptr);
    MFA_ASSERT(entity != nullptr);
    UI::Button("Remove component", [this, entity, component]()
    {
        if (TransformComponent::Name == component->GetName())
        {
            MFA_LOG_WARN("Transform components cannot be removed");
            return;
        }

        auto const findResult = mComponentsDependents.find(component->GetName());
        // Invalid assert because component may not have any dependant
        //MFA_ASSERT(findResult != mComponentsDependents.end());
        for (auto const dependencyName : findResult->second)
        {
            auto dependentComponent = entity->GetComponent(dependencyName);
            if (dependentComponent != nullptr)
            {
                MFA_LOG_WARN(
                    "Cannot delete component with name %s because a dependant component with name %s exists"
                    , component->GetName()
                    , dependentComponent->GetName()
                );
                return;
            }
        }

        entity->RemoveComponent(component);
        EntitySystem::UpdateEntity(entity);
    });
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::prepareDependencyLists()
{
    // Available components
    mAvailableComponents.emplace_back(TransformComponent::Name);
    mAvailableComponents.emplace_back(MeshRendererComponent::Name);
    mAvailableComponents.emplace_back(BoundingVolumeRendererComponent::Name);
    mAvailableComponents.emplace_back(SphereBoundingVolumeComponent::Name);
    mAvailableComponents.emplace_back(AxisAlignedBoundingBoxComponent::Name);
    mAvailableComponents.emplace_back(ColorComponent::Name);
    mAvailableComponents.emplace_back(PointLightComponent::Name);

    // CreateComponentsRequirements
    mComponentDependencies[TransformComponent::Name] = {};
    mComponentDependencies[MeshRendererComponent::Name] = {
        TransformComponent::Name,
        BoundingVolumeComponent::Name
    };
    mComponentDependencies[BoundingVolumeRendererComponent::Name] = {
        ColorComponent::Name,
        BoundingVolumeComponent::Name
    };
    mComponentDependencies[BoundingVolumeComponent::Name] = {};
    mComponentDependencies[ColorComponent::Name] = {};
    mComponentDependencies[PointLightComponent::Name] = {
        TransformComponent::Name,
        ColorComponent::Name
    };

    // DeleteComponentsRequirements
    auto const exists = [](std::vector<std::string> const & list, std::string const & target)->bool
    {
        for (auto const item : list)
        {
            if (item == target)
            {
                return true;
            }
        }
        return false;
    };
    for (auto & pair : mComponentDependencies)
    {
        auto const dependentComponent = pair.first;
        for (auto const dependencyComponent : pair.second)
        {
            auto & deleteComponentRequirement = mComponentsDependents[dependencyComponent];
            if (exists(deleteComponentRequirement, dependentComponent) == false)
            {
                deleteComponentRequirement.emplace_back(dependentComponent);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

// TODO: Remove this , We should be able to create any component that we want
#define INSERT_INTO_CREATE_COMPONENT_MAP(component, createFunction)                 \
mCreateComponentInstructionMap[component::Name] = ComponentCreationInstruction {    \
    .function = (createFunction),                                                   \
}                                                                                   \

void PrefabEditorScene::prepareCreateComponentInstructionMap()
{
    // TODO: Make all components to be added automatically
    INSERT_INTO_CREATE_COMPONENT_MAP(TransformComponent, [](Entity * entity)
    {
        return entity->AddComponent<TransformComponent>();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(MeshRendererComponent, [this](Entity * entity)->std::shared_ptr<MeshRendererComponent>
    {
        if (mSelectedEssenceIndex < 0 || mSelectedEssenceIndex >= static_cast<int>(mLoadedAssets.size()))
        {
            MFA_LOG_WARN("Cannot create mesh renderer component because no essence selected!");
            return nullptr;
        }

        if (mSelectedPipeline < 0 || mSelectedPipeline >= static_cast<int>(mAllPipelines.size()))
        {
            MFA_LOG_WARN("Cannot create mesh renderer component because no pipeline selected!");
            return nullptr;
        }

        auto * pipeline = mAllPipelines[mSelectedPipeline];
        MFA_ASSERT(pipeline != nullptr);
        auto essenceName = mLoadedAssets[mSelectedEssenceIndex].nameId;
        MFA_ASSERT(essenceName.empty() == false);

        if (pipeline->hasEssence(essenceName) == false)
        {
            MFA_LOG_WARN("Pipeline %s does not support essence with name %s", pipeline->GetName(), essenceName.c_str());
            return nullptr;
        }

        return entity->AddComponent<MeshRendererComponent>(pipeline, essenceName);
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(BoundingVolumeRendererComponent, [this](Entity * entity)
    {
        return entity->AddComponent<BoundingVolumeRendererComponent>();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(SphereBoundingVolumeComponent, [](Entity * entity)
    {
        return entity->AddComponent<SphereBoundingVolumeComponent>(1.0f, true);
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(AxisAlignedBoundingBoxComponent, [](Entity * entity)
    {
        return entity->AddComponent<AxisAlignedBoundingBoxComponent>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            false
        );
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(ColorComponent, [](Entity * entity)
    {
        return entity->AddComponent<ColorComponent>(glm::vec3(1.0f, 0.0f, 0.0f));
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(PointLightComponent, [](Entity * entity)
    {
        return entity->AddComponent<PointLightComponent>(
            1.0f,
            100.0f
        );
    });
}

//-------------------------------------------------------------------------------------------------

Entity * PrefabEditorScene::createPrefabEntity(char const * name, Entity * parent, bool const createTransform)
{
    auto * entity = EntitySystem::CreateEntity(name, parent);
    MFA_ASSERT(entity != nullptr);

    if (createTransform)
    {
        entity->AddComponent<TransformComponent>();
    }

    bindEditorSignalToEntity(entity);

    EntitySystem::InitEntity(entity);

    return entity;
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::bindEditorSignalToEntity(Entity * entity)
{
    MFA_ASSERT(entity != nullptr);
    entity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);

    for (auto * component : entity->GetComponents())
    {
        MFA_ASSERT(component != nullptr);
        component->EditorSignal.Register(this, &PrefabEditorScene::componentUI);
    }
    for (auto * child : entity->GetChildEntities())
    {
        MFA_ASSERT(child != nullptr);
        bindEditorSignalToEntity(child);
    }
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::savePrefab()
{
    std::vector<WinApi::Extension> const extensions{
        WinApi::Extension {
            .name = "Json",
            .value = "*.json"
        }
    };

    std::string filePath{};
    auto const success = WinApi::SaveAs(extensions, filePath);
    if (success)
    {
        auto const extension = Path::ExtractExtensionFromPath(filePath);
        if (extension.empty())
        {
            filePath += ".json";
        }

        printf("Save address is %s", filePath.c_str());
        PrefabFileStorage::Serialize(PrefabFileStorage::SerializeParams{
            .saveAddress = filePath,
            .prefab = &mPrefab
        });
    }
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::loadPrefab()
{
    std::vector<WinApi::Extension> const extensions{
        WinApi::Extension {
            .name = "Json",
            .value = "*.json"
        }
    };

    std::string filePath{};
    auto const success = WinApi::TryToPickFile(extensions, filePath);
    if (success)
    {
        EntitySystem::DestroyEntity(mPrefabRootEntity);

        // TODO: We have to free essences as well

        mPrefabRootEntity = EntitySystem::CreateEntity("RootEntity", GetRootEntity());
        MFA_ASSERT(mPrefabRootEntity != nullptr);

        mPrefab.AssignPreBuiltEntity(mPrefabRootEntity);

        PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams{
            .fileAddress = filePath,
            .prefab = &mPrefab,
            .initializeEntity = true
        });

        bindEditorSignalToEntity(mPrefabRootEntity);

        SceneManager::TriggerCleanup();
    }
}

//-------------------------------------------------------------------------------------------------
