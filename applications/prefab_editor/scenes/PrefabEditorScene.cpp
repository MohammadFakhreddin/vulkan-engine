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

    {// Creating prefab
        mPrefabEntity = EntitySystem::CreateEntity("PrefabInstance", GetRootEntity());
        MFA_ASSERT(mPrefabEntity != nullptr);
        mPrefabEntity->EditorSignal.Register(this, &PrefabEditorScene::entityUI);
        EntitySystem::InitEntity(mPrefabEntity);
    }
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnPreRender(
    float deltaTimeInSec,
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

    destroyPrefabModelAndEssence();

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
    UI::BeginWindow("Menu");
    UI::InputText("Prefab name", mPrefabName);
    UI::EndWindow();
    EntitySystem::OnUI();
}

//-------------------------------------------------------------------------------------------------

bool PrefabEditorScene::loadSelectedAsset(std::string const & fileAddress)
{
    destroyPrefabModelAndEssence();
    MFA_LOG_INFO("Trying to load file with address %s", fileAddress.c_str());
    MFA_ASSERT(fileAddress.empty() == false);
    auto cpuModel = Importer::ImportGLTF(fileAddress.c_str());
    mPrefabGpuModel = RF::CreateGpuModel(cpuModel);
    if (mPrefabGpuModel.valid == false)
    {
        MFA_LOG_WARN("Gltf model is invalid. Failed to create gpu model");
        return false;
    }
    mPbrPipeline.CreateDrawableEssence(PREFAB_ESSENCE, mPrefabGpuModel);
    return true;
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::destroyPrefabModelAndEssence()
{
    if (mPrefabGpuModel.valid == false)
    {
        return;
    }
    RF::DeviceWaitIdle();
    mPbrPipeline.DestroyDrawableEssence(PREFAB_ESSENCE);
    RF::DestroyGpuModel(mPrefabGpuModel);
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
        UI::Button("Add component", [this]()->void
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
                    newComponent = mPrefabEntity->AddComponent<TransformComponent>().lock();
                }
                break;
                case 2:     // MeshRendererComponent
                {
                    if (mPrefabEntity->GetComponent<BoundingVolumeComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating MeshRenderer, BoundingVolumeComponent must exists first!");
                        return;
                    }
                    if (mPrefabEntity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating MeshRenderer, TransformComponent must exists first!");
                        return;
                    }
                    std::string fileAddress = "";
                    if (WinApi::TryToPickFile(fileAddress) == false)
                    {
                        MFA_LOG_INFO("No valid file address picked!");
                        return;
                    }
                    if (loadSelectedAsset(fileAddress) == false) {
                        return;
                    }
                    newComponent = mPrefabEntity->AddComponent<MeshRendererComponent>(mPbrPipeline, PREFAB_ESSENCE).lock();
                }
                break;
                case 3:     // BoundingVolumeRendererComponent
                {
                    if (mPrefabEntity->GetComponent<BoundingVolumeComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating BoundingVolumeRendererComponent, BoundingVolumeComponent must exists first!");
                        return;
                    }
                    if (mPrefabEntity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating BoundingVolumeRendererComponent, ColorComponent must exists first!");
                        return;
                    }
                    newComponent = mPrefabEntity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline).lock();
                }
                break;
                case 4:     // SphereBoundingVolumeComponent
                {
                    newComponent = mPrefabEntity->AddComponent<SphereBoundingVolumeComponent>(1.0f).lock();
                }
                break;
                case 5:     // AxisAlignedBoundingBoxes
                {
                    newComponent = mPrefabEntity->AddComponent<AxisAlignedBoundingBoxComponent>(
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(1.0f, 1.0f, 1.0f)
                    ).lock();
                }
                break;
                case 6:     // ColorComponent
                {
                    newComponent = mPrefabEntity->AddComponent<ColorComponent>(glm::vec3(1.0f, 0.0f, 0.0f)).lock();
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
                    if (mPrefabEntity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a PointLightComponent, the ColorComponent must exists first!");
                        return;
                    }
                    if (mPrefabEntity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a PointLightComponent, the TransformComponent must exists first!");
                        return;
                    }
                    newComponent = mPrefabEntity->AddComponent<PointLightComponent>(
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
                    if (mPrefabEntity->GetComponent<ColorComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a DirectionalLightComponent, the ColorComponent must exists first!");
                        return;
                    }
                    if (mPrefabEntity->GetComponent<TransformComponent>().expired())
                    {
                        MFA_LOG_WARN("For creating a DirectionalLightComponent, the TransformComponent must exists first!");
                        return;
                    }
                    newComponent = mPrefabEntity->AddComponent<DirectionalLightComponent>().lock();
                }
                break;
                default:
                    MFA_LOG_WARN("Unhandled component type detected! %d", mSelectedComponentIndex);
            }

            newComponent->Init();
            EntitySystem::UpdateEntity(mPrefabEntity);

            newComponent->EditorSignal.Register(this, &PrefabEditorScene::componentUI);
            
            mSelectedComponentIndex = 0;
        });
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
