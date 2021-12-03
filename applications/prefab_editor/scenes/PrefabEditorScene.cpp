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
#include "engine/camera/ThirdPersonCameraComponent.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::PrefabEditorScene() = default;

//-------------------------------------------------------------------------------------------------

PrefabEditorScene::~PrefabEditorScene() = default;

//-------------------------------------------------------------------------------------------------
// TODO Start from here. Write data in json and load it again
void PrefabEditorScene::Init()
{
    Scene::Init();

    // Available components
    mAvailableComponents.emplace_back(TransformComponent::Name);
    mAvailableComponents.emplace_back(MeshRendererComponent::Name);
    mAvailableComponents.emplace_back(BoundingVolumeRendererComponent::Name);
    mAvailableComponents.emplace_back(SphereBoundingVolumeComponent::Name);
    mAvailableComponents.emplace_back(AxisAlignedBoundingBoxComponent::Name);
    mAvailableComponents.emplace_back(ColorComponent::Name);
    mAvailableComponents.emplace_back(ObserverCameraComponent::Name);
    mAvailableComponents.emplace_back(ThirdPersonCameraComponent::Name);
    mAvailableComponents.emplace_back(PointLightComponent::Name);
    mAvailableComponents.emplace_back(DirectionalLightComponent::Name);

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
        mSphereModel = RC::Assign(sphereCpuModel, "Sphere");
        mDebugRenderPipeline.CreateDrawableEssence(mSphereModel);

        auto cubeCpuModel = ShapeGenerator::Cube();
        mCubeModel = RC::Assign(cubeCpuModel, "Cube");
        mDebugRenderPipeline.CreateDrawableEssence(mCubeModel);
    }

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    {// Creating observer camera and directional light
        auto * entity = EntitySystem::CreateEntity("CameraEntity", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto observerCamera = entity->AddComponent<ObserverCameraComponent>(FOV, Z_NEAR, Z_FAR).lock();
        MFA_ASSERT(observerCamera != nullptr);
        SetActiveCamera(observerCamera);

        entity->AddComponent<DirectionalLightComponent>();

        auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
        MFA_ASSERT(colorComponent != nullptr);
        float lightScale = 5.0f;
        float lightColor[3] {
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

    /*for (int i = static_cast<int>(mLoadedAssets.size()) - 1; i >= 0; --i)
    {
        destroyAsset(i);
    }*/

    //mCubeModel.reset();
    //mSphereModel.reset();

    //mPbrPipeline.Shutdown();
    //mDebugRenderPipeline.Shutdown();
    //mLoadedAssets.clear();

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

void PrefabEditorScene::saveAndLoadWindow()
{
    UI::BeginWindow("Save and load panel");
    UI::InputText("Prefab name", mPrefabName);
    if (UI::TreeNode("Save"))
    {
        UI::TreePop();    
    }
    if (UI::TreeNode("Load"))
    {
        UI::TreePop();
    }
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

bool PrefabEditorScene::loadSelectedAsset(std::string const & fileAddress)
{
    MFA_LOG_INFO("Trying to load file with address %s", fileAddress.c_str());
    MFA_ASSERT(fileAddress.empty() == false);

    auto gpuModel = ResourceManager::Acquire(fileAddress.c_str());
    if (gpuModel->valid == false)
    {
        MFA_LOG_WARN("Gltf model is invalid. Failed to create gpu model");
        return false;
    }

    mLoadedAssets.emplace_back(Asset {
        .fileAddress = fileAddress,
        .essenceName = mInputTextEssenceName,
        .gpuModel = gpuModel    // TODO We need shared_ptr + weak_ptr
    });

    mPbrPipeline.CreateDrawableEssence(gpuModel);

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

            // TODO I need a dependency graph for both create and remove

            std::shared_ptr<Component> newComponent {};

            auto const newComponentName = mAvailableComponents[mSelectedComponentIndex];
            if (newComponentName == TransformComponent::Name)
            {
                newComponent = entity->AddComponent<TransformComponent>().lock();
            }
            else if (newComponentName == MeshRendererComponent::Name)
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
                    mLoadedAssets[mSelectedEssenceIndex].gpuModel->id
                ).lock();
            }
            else if (newComponentName == BoundingVolumeRendererComponent::Name)
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
            else if (newComponentName == SphereBoundingVolumeComponent::Name)
            {
                newComponent = entity->AddComponent<SphereBoundingVolumeComponent>(1.0f).lock();
            }
            else if (newComponentName == AxisAlignedBoundingBoxComponent::Name)
            {
                newComponent = entity->AddComponent<AxisAlignedBoundingBoxComponent>(
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f)
                ).lock();
            }
            else if (newComponentName == ColorComponent::Name)
            {
                newComponent = entity->AddComponent<ColorComponent>(glm::vec3(1.0f, 0.0f, 0.0f)).lock();
            }
            else if (newComponentName == ObserverCameraComponent::Name)
            {   // I should delete options of creating components that creating them is unhelpful
                MFA_LOG_WARN("ObserverCameraComponent will not be supported");
            }
            else if (newComponentName == ThirdPersonCameraComponent::Name)
            {
                MFA_LOG_WARN("ThirdPersonCamera is not supported yet");
            }
            else if (newComponentName == PointLightComponent::Name)
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
            else if (newComponentName == DirectionalLightComponent::Name)
            {
                MFA_LOG_WARN("Defining directional light is not recomended");
                /* if (entity->GetComponent<ColorComponent>().expired())
                {
                    MFA_LOG_WARN("For creating a DirectionalLightComponent, the ColorComponent must exists first!");
                    return;
                }
                if (entity->GetComponent<TransformComponent>().expired())
                {
                    MFA_LOG_WARN("For creating a DirectionalLightComponent, the TransformComponent must exists first!");
                    return;
                }
                newComponent = entity->AddComponent<DirectionalLightComponent>().lock();*/
            }
            else
            {
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

Entity * PrefabEditorScene::createPrefabEntity(char const * name, Entity * parent)
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
