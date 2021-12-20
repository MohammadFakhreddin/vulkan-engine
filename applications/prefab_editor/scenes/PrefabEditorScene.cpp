#include "PrefabEditorScene.hpp"

#include "WindowsApi.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"
#include "engine/camera/ObserverCameraComponent.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "tools/PrefabFileStorage.hpp"
#include "engine/BedrockPath.hpp"

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

    {// Error texture
        auto cpuTexture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpuTexture);
    }

    // Sampler
    mSampler = RF::CreateSampler(RT::CreateSamplerParams{});

    // Pbr pipeline
    mPbrPipeline.Init(mSampler, mErrorTexture);

    {// Debug renderer pipeline
        mDebugRenderPipeline.Init();

        auto const sphereCpuModel = ShapeGenerator::Sphere();
        auto sphereModel = RC::Assign(sphereCpuModel, "Sphere");
        mDebugRenderPipeline.CreateEssenceIfNotExists(sphereModel);

        mLoadedAssets.emplace_back(Asset {
            .fileAddress = "",
            .essenceName = "Sphere",
            .gpuModel = std::move(sphereModel)
        });

        auto const cubeCpuModel = ShapeGenerator::Cube();
        auto cubeModel = RC::Assign(cubeCpuModel, "Cube");
        mDebugRenderPipeline.CreateEssenceIfNotExists(cubeModel);

        mLoadedAssets.emplace_back(Asset {
            .fileAddress = "",
            .essenceName = "Cube",
            .gpuModel = std::move(cubeModel)
        });
    }

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    {// Creating observer camera and directional light
        auto * entity = EntitySystem::CreateEntity("CameraEntity", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const observerCamera = entity->AddComponent<ObserverCameraComponent>(FOV, Z_NEAR, Z_FAR).lock();
        MFA_ASSERT(observerCamera != nullptr);
        SetActiveCamera(observerCamera);

        entity->AddComponent<DirectionalLightComponent>();

        auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
        MFA_ASSERT(colorComponent != nullptr);
        float const lightScale = 5.0f;
        float lightColor[3]{
            1.0f * lightScale,
            1.0f * lightScale,
            1.0f * lightScale
        };
        colorComponent->SetColor(lightColor);

        auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdateRotation(glm::vec3(90.0f, 0.0f, 0.0f));

        entity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);
        EntitySystem::InitEntity(entity);
    }

    mPrefabRootEntity = createPrefabEntity("PrefabRoot", GetRootEntity());

    mPrefab.AssignPreBuiltEntity(mPrefabRootEntity);

    RegisterPipeline(&mDebugRenderPipeline);
    RegisterPipeline(&mPbrPipeline);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnPreRender(
    float const deltaTimeInSec,
    RT::CommandRecordState & recordState
)
{
    Scene::OnPreRender(deltaTimeInSec, recordState);

    mDebugRenderPipeline.PreRender(recordState, deltaTimeInSec);
    mPbrPipeline.PreRender(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnRender(
    float const deltaTimeInSec,
    RT::CommandRecordState & recordState
)
{
    Scene::OnRender(deltaTimeInSec, recordState);

    mDebugRenderPipeline.Render(recordState, deltaTimeInSec);
    mPbrPipeline.Render(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnPostRender(
    float const deltaTimeInSec,
    RT::CommandRecordState & recordState
)
{
    Scene::OnPostRender(deltaTimeInSec, recordState);

    mDebugRenderPipeline.PostRender(recordState, deltaTimeInSec);
    mPbrPipeline.PostRender(recordState, deltaTimeInSec);

}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::Shutdown()
{
    Scene::Shutdown();
    UI::UnRegister(mUIRecordId);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnResize()
{
    mPbrPipeline.OnResize();
    mDebugRenderPipeline.OnResize();
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
            if (UI::TreeNode(asset.essenceName.c_str()))
            {
                UI::Text(asset.fileAddress.c_str());
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
            std::string fileAddress;
            if (WinApi::TryToPickFile(extensions, fileAddress) == false)
            {
                MFA_LOG_WARN("No valid file address picked!");
                return;
            }
            bool const success = Path::RelativeToAssetFolder(fileAddress.c_str(), fileAddress);
            if (success == false)
            {
                MFA_LOG_WARN("All assets and prefabs must be placed in asset folder for portability\n");
                return;
            }
            if (loadSelectedAsset(fileAddress) == false)
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
            auto const extension = FileSystem::ExtractExtensionFromPath(filePath.c_str());
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
    });
    UI::Button("Load", [this]()->void
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

            mPrefabRootEntity = EntitySystem::CreateEntity("RootEntity", GetRootEntity());
            MFA_ASSERT(mPrefabRootEntity != nullptr);

            mPrefab.AssignPreBuiltEntity(mPrefabRootEntity);

            PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams{
                .fileAddress = filePath,
                .prefab = &mPrefab,
                .initializeEntity = true
            });

            bindEditorSignalToEntity(mPrefabRootEntity);
        }
    });

    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

bool PrefabEditorScene::loadSelectedAsset(std::string const & fileAddress)
{
    MFA_LOG_INFO("Trying to load file with address %s", fileAddress.c_str());
    MFA_ASSERT(fileAddress.empty() == false);

    auto gpuModel = ResourceManager::Acquire(fileAddress.c_str());
    if (gpuModel == nullptr)
    {
        MFA_LOG_WARN("Gltf model is invalid. Failed to create gpu model");
        return false;
    }

    mLoadedAssets.emplace_back(Asset{
        .fileAddress = fileAddress,
        .essenceName = mInputTextEssenceName,
        .gpuModel = gpuModel    // TODO We need shared_ptr + weak_ptr
    });

    mPbrPipeline.CreateEssenceIfNotExists(gpuModel);

    mInputTextEssenceName = "";

    return true;
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::destroyAsset(int const assetIndex)
{
    auto & asset = mLoadedAssets[assetIndex];
    mPbrPipeline.DestroyDrawableEssence(*asset.gpuModel);
    asset.gpuModel.reset();
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
            for (auto const * pipeline : GetPipelines())
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
                essenceNames.emplace_back(asset.essenceName);
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

            auto const checkForDependencies = [this, entity, &newComponentName](int const familyType)->bool
            {
                auto const findResult = mComponentDependencies.find(familyType);
                MFA_ASSERT(findResult != mComponentDependencies.end());
                for (auto const familyId : findResult->second)
                {
                    auto dependencyComponent = entity->GetComponent(familyId).lock();
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
                if (checkForDependencies(findCreateInstructionResult->second.familyType))
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

        auto const findResult = mComponentsDependents.find(component->GetFamilyType());
        MFA_ASSERT(findResult != mComponentsDependents.end());
        for (auto const familyId : findResult->second)
        {
            auto dependentComponent = entity->GetComponent(familyId).lock();
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
    mComponentDependencies[TransformComponent::FamilyType] = {};
    mComponentDependencies[MeshRendererComponent::FamilyType] = {
        TransformComponent::FamilyType,
        BoundingVolumeComponent::FamilyType
    };
    mComponentDependencies[BoundingVolumeRendererComponent::FamilyType] = {
        ColorComponent::FamilyType,
        BoundingVolumeComponent::FamilyType
    };
    mComponentDependencies[BoundingVolumeComponent::FamilyType] = {};
    mComponentDependencies[ColorComponent::FamilyType] = {};
    mComponentDependencies[PointLightComponent::FamilyType] = {
        TransformComponent::FamilyType,
        ColorComponent::FamilyType
    };

    // DeleteComponentsRequirements
    auto const exists = [](std::vector<int> const & list, int const target)->bool
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

#define INSERT_INTO_CREATE_COMPONENT_MAP(component, createFunction)             \
mCreateComponentInstructionMap[component::Name] = CreateComponentInstruction {  \
    .familyType = component::FamilyType,                                        \
    .function = (createFunction),                                               \
}                                                                               \

void PrefabEditorScene::prepareCreateComponentInstructionMap()
{
    INSERT_INTO_CREATE_COMPONENT_MAP(TransformComponent, [](Entity * entity)
    {
        return entity->AddComponent<TransformComponent>().lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(MeshRendererComponent, [this](Entity * entity)->std::shared_ptr<MeshRendererComponent>
    {
        if (mSelectedEssenceIndex < 0 || mSelectedEssenceIndex >= static_cast<int>(mLoadedAssets.size()))
        {
            return nullptr;
        }
        if (mSelectedPipeline < 0 || mSelectedPipeline >= static_cast<int>(GetPipelines().size()))
        {
            return nullptr;
        }
        return entity->AddComponent<MeshRendererComponent>(
            *GetPipelines()[mSelectedPipeline],
            mLoadedAssets[mSelectedEssenceIndex].gpuModel->id
        ).lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(BoundingVolumeRendererComponent, [this](Entity * entity)
    {
        return  entity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline).lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(SphereBoundingVolumeComponent, [](Entity * entity)
    {
        return entity->AddComponent<SphereBoundingVolumeComponent>(1.0f).lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(AxisAlignedBoundingBoxComponent, [](Entity * entity)
    {
        return entity->AddComponent<AxisAlignedBoundingBoxComponent>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        ).lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(ColorComponent, [](Entity * entity)
    {
        return entity->AddComponent<ColorComponent>(glm::vec3(1.0f, 0.0f, 0.0f)).lock();
    });

    INSERT_INTO_CREATE_COMPONENT_MAP(PointLightComponent, [](Entity * entity)
    {
        return entity->AddComponent<PointLightComponent>(
            1.0f,
            100.0f,
            Z_NEAR,
            Z_FAR
        ).lock();
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
