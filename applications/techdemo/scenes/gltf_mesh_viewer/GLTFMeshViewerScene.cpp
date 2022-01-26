#include "GLTFMeshViewerScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

GLTFMeshViewerScene::GLTFMeshViewerScene()
    : Scene()
{}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::Init() {
    Scene::Init();
    {// Error texture
        auto cpu_texture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(*cpu_texture);
    }
    {// Models
        //{
        //    ModelRenderRequiredData params {};
        //    params.displayName = "Gunner";
        //    Path::Asset("models/jade/gunner.gltf", params.address);
        //    MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {0.0f, 0.0f, -180.0f});
        //    MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -7.0f});        
        //    MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, -2.0f});
        //    MFA::Copy<3>(params.initialParams.camera.position, {0.104f, 1.286f, 4.952f});
        //    MFA::Copy<3>(params.initialParams.camera.eulerAngles, {-12.0f, 3.0f, 0.0f});
        //    mModelsRenderData.emplace_back(params);
        //}
        {
            ModelRenderRequiredData params {};
            params.displayName = "CesiumMan";
            Path::ForReadWrite("models/CesiumMan/glTF/CesiumMan.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {0.0f, 0.0f, -180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -7.0f});        
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, -2.0f});
            MFA::Copy<3>(params.initialParams.camera.position, {0.104f, 1.286f, 4.952f});
            MFA::Copy<3>(params.initialParams.camera.eulerAngles, {-12.0f, 3.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.gpuModel = nullptr;
            params.displayName = "War-craft soldier";
            Path::ForReadWrite("models/warcraft_3_alliance_footmanfanmade/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {0.0f, -5.926f, -180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.9f, -2.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "SponzaScene";
            Path::ForReadWrite("models/sponza/sponza.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, -90.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.4f, 2.0f, -6.0f});
            MFA::Copy<3>(params.initialParams.light.translateMin, {-50.0f, -50.0f, -50.0f});
            MFA::Copy<3>(params.initialParams.light.translateMax, {50.0f, 50.0f, 50.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Mira";
            Path::ForReadWrite("models/mira/scene.gltf", params.address);
            params.initialParams.model.scale = 0.005f;
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.9f, -2.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.camera.position, {0.5f, 1.1f, -1.3f});
            MFA::Copy<3>(params.initialParams.camera.eulerAngles, {-13.0f, 0.0f, 0.0f});

            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Car";
            Path::ForReadWrite("models/free_zuk_3d_model/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-19.0f, -32.0f, 177.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -2.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Cyberpunk lady";
            Path::ForReadWrite("models/female_full-body_cyberpunk_themed_avatar/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-3.0f, 340.0f, 180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.8f, -3.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Mandalorian";
            Path::ForReadWrite("models/fortnite_the_mandalorianbaby_yoda/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            params.initialParams.model.scale = 0.010f;
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, -0.5f, -2.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, 0.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Flight helmet";
            Path::ForReadWrite("models/FlightHelmet/glTF/FlightHelmet.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -1.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, 0.0f, 2.469f});
            mModelsRenderData.emplace_back(params);
        }
    }

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler(MFA::RT::CreateSamplerParams {});
    
    mDebugRenderPipeline.Init();

    {// Camera
        auto * entity = EntitySystem::CreateEntity("Camera", GetRootEntity());

        mCamera = entity->AddComponent<ObserverCameraComponent>(
            FOV,
            Z_NEAR,
            Z_FAR
        );

        MFA_ASSERT(mCamera.expired() == false);

        EntitySystem::InitEntity(entity);

        SetActiveCamera(mCamera);
    }
    
    mPbrPipeline.Init(mSamplerGroup, mErrorTexture);

    {// Point light
        auto * entity = EntitySystem::CreateEntity("PointLight", GetRootEntity());

        auto const colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent.expired() == false);
        mPointLightColor = colorComponent;

        auto const transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent.expired() == false);
        if (auto const ptr = transformComponent.lock())
        {
            ptr->UpdateScale(glm::vec3(0.1f, 0.1f, 0.1f));
        }

        entity->AddComponent<PointLightComponent>(0.5f, 1000.0f);

        mPointLightTransform = transformComponent;

        entity->AddComponent<MeshRendererComponent>(mDebugRenderPipeline, "Sphere");

        entity->AddComponent<SphereBoundingVolumeComponent>(0.1f);

        EntitySystem::InitEntity(entity);
    }

    mUIRegisterId = UI::Register([this]()->void {OnUI();});

    RegisterPipeline(&mPbrPipeline);
    RegisterPipeline(&mDebugRenderPipeline);
}

void GLTFMeshViewerScene::OnPreRender(float const deltaTimeInSec, RT::CommandRecordState & recordState) {
    Scene::OnPreRender(deltaTimeInSec, recordState);

    auto & selectedModel = mModelsRenderData[mSelectedModelIndex];

    if (selectedModel.isLoaded == false) {
        createModel(selectedModel);
    }

    if (mPreviousModelSelectedIndex != mSelectedModelIndex) {
        {// Enabling ui for current model
            auto * entity = mModelsRenderData[mSelectedModelIndex].entity;
            MFA_ASSERT(entity != nullptr);
            entity->SetActive(true);
        }
        if (mPreviousModelSelectedIndex >= 0) {// Disabling ui for previous model
            auto * entity = mModelsRenderData[mPreviousModelSelectedIndex].entity;
            MFA_ASSERT(entity != nullptr);
            entity->SetActive(false);
        }

        mPreviousModelSelectedIndex = mSelectedModelIndex;
       
        if (auto const ptr = selectedModel.transformComponent.lock())
        {
            float scale[3]{
                selectedModel.initialParams.model.scale,
                selectedModel.initialParams.model.scale,
                selectedModel.initialParams.model.scale
            };
            ptr->UpdateTransform(
                selectedModel.initialParams.model.translate,
                selectedModel.initialParams.model.rotationEulerAngle,
                scale
            );
        }

        if (auto const ptr = mPointLightTransform.lock())
        {
            ptr->UpdatePosition(selectedModel.initialParams.light.position);
        }

        if (auto const ptr = mPointLightColor.lock())
        {
            ptr->SetColor(selectedModel.initialParams.light.color);
        }

        if (auto const ptr = mCamera.lock())
        {
            ptr->ForcePosition(selectedModel.initialParams.camera.position);
            ptr->ForceRotation(selectedModel.initialParams.camera.eulerAngles);
        }
    }

    mPbrPipeline.PreRender(recordState, deltaTimeInSec);
    mDebugRenderPipeline.PreRender(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnRender(float const deltaTimeInSec, RT::CommandRecordState & recordState) {
    Scene::OnRender(deltaTimeInSec, recordState);

    MFA_ASSERT(mSelectedModelIndex >= 0 && mSelectedModelIndex < static_cast<int32_t>(mModelsRenderData.size()));

    // TODO Pipeline should be able to share buffers such as projection buffer to enable us to update them once -> Solution: Store camera buffers inside camera
    mPbrPipeline.Render(recordState, deltaTimeInSec);
    if (mIsLightVisible) {
        mDebugRenderPipeline.Render(recordState, deltaTimeInSec);
    }
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnPostRender(float const deltaTimeInSec)
{
    Scene::OnPostRender(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnUI() {
    static constexpr float ItemWidth = 500;
    UI::BeginWindow("Scene Subsystem");
    UI::SetNextItemWidth(ItemWidth);
    // TODO Bad for performance, Find a better name
    std::vector<char const *> modelNames {};
    if(false == mModelsRenderData.empty()) {
        for(auto const & renderData : mModelsRenderData) {
            modelNames.emplace_back(renderData.displayName.c_str());
        }
    }
    UI::Combo(
        "Object selector",
        &mSelectedModelIndex,
        modelNames.data(), 
        static_cast<int32_t>(modelNames.size())
    );
    UI::SetNextItemWidth(ItemWidth);
    UI::Button("Reset values", [this]()->void{
        mPreviousModelSelectedIndex = -1;
    });
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::Shutdown() {
    Scene::Shutdown();
    UI::UnRegister(mUIRegisterId);
}

//-------------------------------------------------------------------------------------------------

bool GLTFMeshViewerScene::isDisplayPassDepthImageInitialLayoutUndefined()
{
    return false;
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::createModel(ModelRenderRequiredData & renderRequiredData) {
    auto const cpuModel = RC::AcquireCpuModel(renderRequiredData.address);
    renderRequiredData.gpuModel = RC::AcquireGpuModel(renderRequiredData.address);
    MFA_ASSERT(mPbrPipeline.EssenceExists(renderRequiredData.address) == false);
    mPbrPipeline.CreateEssence(renderRequiredData.gpuModel, cpuModel->mesh);

    auto * entity = EntitySystem::CreateEntity(renderRequiredData.displayName, GetRootEntity());
    MFA_ASSERT(entity != nullptr);
    renderRequiredData.entity = entity;

    renderRequiredData.transformComponent = entity->AddComponent<TransformComponent>();
    MFA_ASSERT(renderRequiredData.transformComponent.expired() == false);

    renderRequiredData.meshRendererComponent = entity->AddComponent<MeshRendererComponent>(
        mPbrPipeline,
        renderRequiredData.gpuModel->nameOrAddress
    );
    MFA_ASSERT(renderRequiredData.meshRendererComponent.expired() == false);

    entity->AddComponent<AxisAlignedBoundingBoxComponent>(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(100.0f, 100.0f, 100.0f)
    );
    
    EntitySystem::InitEntity(entity);
        
    renderRequiredData.isLoaded = true;
}

//-------------------------------------------------------------------------------------------------
