#include "PrefabEditorScene.hpp"

#include "WindowsApi.hpp"
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

using namespace MFA;

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::PrefabEditorScene() = default;

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::~PrefabEditorScene() = default;

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::Init()
{
    Scene::Init();

    {// Error texture
        auto cpuTexture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpuTexture);
    }

    // Sampler
    mSampler = RF::CreateSampler(RT::CreateSamplerParams{});

    // Pbr pipeline
    mPbrPipeline.Init(&mSampler, &mErrorTexture);

    {// Debug renderer pipeline
        mDebugRenderPipeline.Init();

        auto sphereCpuModel = ShapeGenerator::Sphere();
        mSphereModel = RF::CreateGpuModel(sphereCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Sphere", mSphereModel);

        auto cubeCpuModel = ShapeGenerator::Cube();
        mCubeModel = RF::CreateGpuModel(cubeCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Cube", mCubeModel);
    }

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    {// Creating observer camera
        auto * entity = EntitySystem::CreateEntity("CameraEntity", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto observerCamera = entity->AddComponent<ObserverCameraComponent>(FOV, Z_NEAR, Z_FAR).lock();
        MFA_ASSERT(observerCamera != nullptr);
        SetActiveCamera(observerCamera);

        entity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);
        EntitySystem::InitEntity(entity);
    }

    mPrefabRootEntity = createPrefabEntity("PrefabRoot", GetRootEntity());
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

    {// Sphere
        RF::DestroyGpuModel(mSphereModel);
        Importer::FreeModel(mSphereModel.model);
    }
    {// Cube
        RF::DestroyGpuModel(mCubeModel);
        Importer::FreeModel(mCubeModel.model);
    }

    for (int i = static_cast<int>(mLoadedAssets.size()) - 1; i >= 0; --i)
    {
        destroyAsset(i);
    }

    mPbrPipeline.Shutdown();
    mDebugRenderPipeline.Shutdown();
    RF::DestroySampler(mSampler);
    RF::DestroyTexture(mErrorTexture);
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
    UI::BeginWindow("Save and load panel");
    UI::InputText("Prefab name", mPrefabName);
    UI::EndWindow();
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
        UI::Button("Create", [this]()->void{
            std::string fileAddress;
            if (WinApi::TryToPickFile(fileAddress) == false)
            {
                MFA_LOG_INFO("No valid file address picked!");
                return;
            }
            if (loadSelectedAsset(fileAddress) == false) {
                return;
            }
        });
        UI::TreePop();
    }
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

bool PrefabEditorScene::loadSelectedAsset(std::string const & fileAddress)
{
    MFA_LOG_INFO("Trying to load file with address %s", fileAddress.c_str());
    MFA_ASSERT(fileAddress.empty() == false);

    auto cpuModel = Importer::ImportGLTF(fileAddress.c_str());

    auto gpuModel = RF::CreateGpuModel(cpuModel);
    if (gpuModel.valid == false)
    {
        MFA_LOG_WARN("Gltf model is invalid. Failed to create gpu model");
        return false;
    }

    mLoadedAssets.emplace_back(Asset {
        .fileAddress = fileAddress,
        .essenceName = mInputTextEssenceName,
        .gpuModel = gpuModel    // TODO We need shared_ptr + weak_ptr
    });

    mPbrPipeline.CreateDrawableEssence(mInputTextEssenceName.c_str(), mLoadedAssets.back().gpuModel);

    mInputTextEssenceName = "";

    return true;
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::destroyAsset(int const assetIndex)
{
    auto & asset = mLoadedAssets[assetIndex];
    mPbrPipeline.DestroyDrawableEssence(asset.essenceName.c_str());
    RF::DestroyGpuModel(asset.gpuModel);
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
        switch(mSelectedComponentIndex)
        {
            case 2: // MeshRendererComponent
            {
                std::vector<std::string> essenceNames {};
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

            // TODO Check if component already exists

            std::shared_ptr<Component> newComponent {};

            switch (mSelectedComponentIndex)
            {
                case 1:     // TransformComponent
                {
                    newComponent = entity->AddComponent<TransformComponent>().lock();
                }
                break;
                case 2:     // MeshRendererComponent
                {
                    if (entity->GetComponent<BoundingVolumeComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating MeshRenderer, BoundingVolumeComponent must exists first!");
                        return;
                    }
                    if (entity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating MeshRenderer, TransformComponent must exists first!");
                        return;
                    }
                    newComponent = entity->AddComponent<MeshRendererComponent>(
                        mPbrPipeline,
                        mLoadedAssets[mSelectedEssenceIndex].essenceName.c_str()
                    ).lock();
                }
                break;
                case 3:     // BoundingVolumeRendererComponent
                {
                    if (entity->GetComponent<BoundingVolumeComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating BoundingVolumeRendererComponent, BoundingVolumeComponent must exists first!");
                        return;
                    }
                    if (entity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating BoundingVolumeRendererComponent, ColorComponent must exists first!");
                        return;
                    }
                    newComponent = entity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline).lock();
                }
                break;
                case 4:     // SphereBoundingVolumeComponent
                {
                    newComponent = entity->AddComponent<SphereBoundingVolumeComponent>(1.0f).lock();
                }
                break;
                case 5:     // AxisAlignedBoundingBoxes
                {
                    newComponent = entity->AddComponent<AxisAlignedBoundingBoxComponent>(
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(1.0f, 1.0f, 1.0f)
                    ).lock();
                }
                break;
                case 6:     // ColorComponent
                {
                    newComponent = entity->AddComponent<ColorComponent>(glm::vec3(1.0f, 0.0f, 0.0f)).lock();
                }
                break;
                case 7:     // ObserverCameraComponent
                {
                    MFA_LOG_WARN("ObserverCameraComponent is not supported yet");
                }
                break;
                case 8:     // ThirdPersonCamera
                {
                    MFA_LOG_WARN("ThirdPersonCamera is not supported yet");
                }
                break;
                case 9:     // PointLightComponent
                {
                    if (entity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a PointLightComponent, the ColorComponent must exists first!");
                        return;
                    }
                    if (entity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a PointLightComponent, the TransformComponent must exists first!");
                        return;
                    }
                    newComponent = entity->AddComponent<PointLightComponent>(
                        1.0f,
                        100.0f,
                        Z_NEAR,
                        Z_FAR
                    ).lock();
                    // What should we do for attached variant? I think it is not possible at the moment
                }
                break;
                case 10:    // DirectionalLightComponent
                {
                    if (entity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a DirectionalLightComponent, the ColorComponent must exists first!");
                        return;
                    }
                    if (entity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a DirectionalLightComponent, the TransformComponent must exists first!");
                        return;
                    }
                    newComponent = entity->AddComponent<DirectionalLightComponent>().lock();
                }
                break;
                default:
                    MFA_LOG_WARN("Unhandled component type detected! %d", mSelectedComponentIndex);
            }

            if (newComponent == nullptr)
            {
                return;
            }
            newComponent->Init();
            EntitySystem::UpdateEntity(entity);

            newComponent->EditorSignal.Register(this, &PrefabEditorScene::componentUI);
            
            mSelectedComponentIndex = 0;
        });
        if (UI::TreeNode("Add child"))
        {
            UI::InputText("ChildName", mInputChildEntityName);
            UI::Button("Add", [entity, this]()->void{
                createPrefabEntity(mInputChildEntityName.c_str(), entity);
                mInputChildEntityName = "";
            });
            UI::TreePop();
        }
        if (UI::TreeNode("Remove entity"))
        {
            UI::Button("Remove", [entity, this]()->void{
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
    UI::Button("Remove component", [this, entity, component]()->void{
        entity->RemoveComponent(component);
        EntitySystem::UpdateEntity(entity);
    });
}

//-------------------------------------------------------------------------------------------------

Entity *  PrefabEditorScene::createPrefabEntity(char const * name, Entity * parent)
{
    auto * entity = EntitySystem::CreateEntity(name, parent);
    MFA_ASSERT(entity != nullptr);
    entity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);

    auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
    transformComponent->EditorSignal.Register(this, &PrefabEditorScene::componentUI);

    EntitySystem::InitEntity(entity);

    return entity;
}

//-------------------------------------------------------------------------------------------------
