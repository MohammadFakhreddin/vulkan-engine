#include "GLTFMeshViewerScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
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
        mErrorTexture = RF::CreateTexture(cpu_texture);
    }
    {// Models
        {
            ModelRenderRequiredData params {};
            params.displayName = "CesiumMan";
            Path::Asset("models/CesiumMan/glTF/CesiumMan.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {0.0f, 0.0f, -180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -7.0f});        
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, -2.0f});
            MFA::Copy<3>(params.initialParams.camera.position, {0.104f, 1.286f, 4.952f});
            MFA::Copy<3>(params.initialParams.camera.eulerAngles, {-12.0f, 3.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.gpuModel = {};
            params.displayName = "War-craft soldier";
            Path::Asset("models/warcraft_3_alliance_footmanfanmade/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {0.0f, -5.926f, -180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.9f, -2.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, -2.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "SponzaScene";
            Path::Asset("models/sponza/sponza.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, -90.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.4f, 2.0f, -6.0f});
            MFA::Copy<3>(params.initialParams.light.translateMin, {-50.0f, -50.0f, -50.0f});
            MFA::Copy<3>(params.initialParams.light.translateMax, {50.0f, 50.0f, 50.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Mira";
            Path::Asset("models/mira/scene.gltf", params.address);
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
            Path::Asset("models/free_zuk_3d_model/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-19.0f, -32.0f, 177.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -2.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Cyberpunk lady";
            Path::Asset("models/female_full-body_cyberpunk_themed_avatar/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-3.0f, 340.0f, 180.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.8f, -3.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Mandalorian";
            Path::Asset("models/fortnite_the_mandalorianbaby_yoda/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            params.initialParams.model.scale = 0.010f;
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, -0.5f, -2.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, 0.0f, 0.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Flight helmet";
            Path::Asset("models/FlightHelmet/glTF/FlightHelmet.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -1.0f});
            MFA::Copy<3>(params.initialParams.light.position, {0.0f, 0.0f, 2.469f});
            mModelsRenderData.emplace_back(params);
        }
    }

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler(MFA::RT::CreateSamplerParams {});
    
    mPointLightPipeline.Init();

    {// Camera
        auto * entity = EntitySystem::CreateEntity("Camera", GetRootEntity());

        mCamera = entity->AddComponent<ObserverCameraComponent>(
            FOV,
            Z_FAR,
            Z_NEAR
        );
        MFA_ASSERT(mCamera != nullptr);
        EntitySystem::InitEntity(entity);

        SetActiveCamera(mCamera);
    }
    
    mPbrPipeline.Init(&mSamplerGroup, &mErrorTexture, Z_NEAR, Z_FAR);

    // Updating perspective mat once for entire application
    // Perspective
    updateProjectionBuffer();

    {// Point light
        auto cpuModel = MFA::ShapeGenerator::Sphere(0.1f);
        mPointLightModel = RF::CreateGpuModel(cpuModel);
        mPointLightPipeline.CreateDrawableEssence("Sphere", mPointLightModel);

        auto * entity = EntitySystem::CreateEntity("PointLight", GetRootEntity());

        auto * colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent != nullptr);
        colorComponent->SetColor(mLightColor);

        mPointLightColor = colorComponent;

        auto * transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdatePosition(mLightPosition);

        mPointLightTransform = transformComponent;

        auto * meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPointLightPipeline, "Sphere");
        MFA_ASSERT(meshRendererComponent != nullptr);

        entity->AddComponent<SphereBoundingVolumeComponent>(0.1f);

        EntitySystem::InitEntity(entity);
    }

    mUIRegisterId = UI::Register([this]()->void {OnUI();});
}

void GLTFMeshViewerScene::OnPreRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) {
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
        // Model
        MFA::Copy<3>(mModelRotation, selectedModel.initialParams.model.rotationEulerAngle);
        MFA::Copy<3>(mModelPosition, selectedModel.initialParams.model.translate);
        mModelScale = selectedModel.initialParams.model.scale;
        MFA::Copy<3>(mModelTranslateMin, selectedModel.initialParams.model.translateMin);
        MFA::Copy<3>(mModelTranslateMax, selectedModel.initialParams.model.translateMax);
        
        // Light
        MFA::Copy<3>(mLightPosition, selectedModel.initialParams.light.position);
        MFA::Copy<3>(mLightColor, selectedModel.initialParams.light.color);
        MFA::Copy<3>(mLightTranslateMin, selectedModel.initialParams.light.translateMin);
        MFA::Copy<3>(mLightTranslateMax, selectedModel.initialParams.light.translateMax);
        
        mCamera->ForcePosition(selectedModel.initialParams.camera.position);
        mCamera->ForceRotation(selectedModel.initialParams.camera.eulerAngles);
    }
    
    {// LightViewBuffer
        mPbrPipeline.UpdateLightPosition(mLightPosition);

        float cameraPosition[3];
        mCamera->GetPosition(cameraPosition);
        mPbrPipeline.UpdateCameraPosition(cameraPosition);

        mPbrPipeline.UpdateLightColor(mLightColor);

        mPointLightTransform->UpdatePosition(mLightPosition);
        mPointLightColor->SetColor(mLightColor);
    }

    {// Updating PBR-Pipeline
        // Model
        float scale[3] {mModelScale, mModelScale, mModelScale};
        selectedModel.transformComponent->UpdateTransform(
            mModelPosition,
            mModelRotation,
            scale
        );

        // View
        float viewData [16];
        mCamera->GetTransform(viewData);
        mPointLightPipeline.UpdateCameraView(viewData);
        mPbrPipeline.UpdateCameraView(viewData);
    }

    mPbrPipeline.PreRender(drawPass, deltaTimeInSec);
    mPointLightPipeline.PreRender(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass) {
    MFA_ASSERT(mSelectedModelIndex >= 0 && mSelectedModelIndex < static_cast<int32_t>(mModelsRenderData.size()));

    // TODO Pipeline should be able to share buffers such as projection buffer to enable us to update them once -> Solution: Store camera buffers inside camera
    mPbrPipeline.Render(drawPass, deltaTimeInSec);
    if (mIsLightVisible) {
        mPointLightPipeline.Render(drawPass, deltaTimeInSec);
    }
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnPostRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass)
{
    mPointLightPipeline.PostRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PostRender(drawPass, deltaTimeInSec);

}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnUI() {
    static constexpr float ItemWidth = 500;
    UI::BeginWindow("Scene Subsystem");
    UI::Spacing();
    UI::Checkbox("Object viewer window", &mIsObjectViewerWindowVisible);
    UI::Spacing();
    UI::Checkbox("Light window", &mIsLightWindowVisible);
    UI::Spacing();
    UI::Checkbox("Camera window", &mIsCameraWindowVisible);
    UI::Spacing();
    UI::Checkbox("Active model", &mIsDrawableVariantWindowVisible);
    UI::Spacing(); UI::Spacing();
    UI::Button("Reset values", [this]()->void{
        mPreviousModelSelectedIndex = -1;
    });
    UI::EndWindow();

    if (mIsObjectViewerWindowVisible) {
        UI::BeginWindow("Object viewer");
        UI::Spacing();
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
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("XDegree", &mModelRotation[0], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDegree", &mModelRotation[1], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDegree", &mModelRotation[2], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("Scale", &mModelScale, 0.0f, 1.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("XDistance", &mModelPosition[0], mModelTranslateMin[0], mModelTranslateMax[0]);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDistance", &mModelPosition[1], mModelTranslateMin[1], mModelTranslateMax[1]);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDistance", &mModelPosition[2], mModelTranslateMin[2], mModelTranslateMax[2]);
        UI::Spacing();
        UI::EndWindow();
    }

    if (mIsLightWindowVisible) {
        UI::BeginWindow("Light");
        UI::SetNextItemWidth(ItemWidth);
        UI::Checkbox("Visible", &mIsLightVisible);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionX", &mLightPosition[0], mLightTranslateMin[0], mLightTranslateMax[0]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionY", &mLightPosition[1], mLightTranslateMin[1], mLightTranslateMax[1]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("PositionZ", &mLightPosition[2], mLightTranslateMin[2], mLightTranslateMax[2]);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorR", &mLightColor[0], 0.0f, 400.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorG", &mLightColor[1], 0.0f, 400.0f);
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ColorB", &mLightColor[2], 0.0f, 400.0f);
        UI::EndWindow();
    }

    if (mIsDrawableVariantWindowVisible) {
        const auto & selectedModel = mModelsRenderData[mSelectedModelIndex];
        // TODO We should not expose variant
        auto * variant = selectedModel.meshRendererComponent->GetVariant();
        if (variant != nullptr) {
            variant->OnUI();
        }
    }

    if (mIsCameraWindowVisible) {
        mCamera->OnUI();
    }

    // TODO Node tree
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::Shutdown() {
    Scene::Shutdown();

    UI::UnRegister(mUIRegisterId);

    //{// Disabling ui for current model
    //    auto * selectedDrawable = mModelsRenderData[mSelectedModelIndex].variant;
    //    MFA_ASSERT(selectedDrawable != nullptr);
    //    selectedDrawable->SetActive(false);
    //}
    mPbrPipeline.Shutdown();
    mPointLightPipeline.Shutdown();
    RF::DestroySampler(mSamplerGroup);
    destroyModels();
    RF::DestroyTexture(mErrorTexture);
    Importer::FreeTexture(mErrorTexture.cpuTexture());
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::OnResize() {
    mCamera->OnResize();
    updateProjectionBuffer();
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::createModel(ModelRenderRequiredData & renderRequiredData) {
    auto cpuModel = Importer::ImportGLTF(renderRequiredData.address.c_str());
    renderRequiredData.gpuModel = RF::CreateGpuModel(cpuModel);
    mPbrPipeline.CreateDrawableEssence(renderRequiredData.displayName.c_str(), renderRequiredData.gpuModel);

    auto * entity = EntitySystem::CreateEntity("Instance", GetRootEntity());
    MFA_ASSERT(entity != nullptr);
    renderRequiredData.entity = entity;

    renderRequiredData.transformComponent = entity->AddComponent<TransformComponent>();
    MFA_ASSERT(renderRequiredData.transformComponent != nullptr);

    renderRequiredData.meshRendererComponent = entity->AddComponent<MeshRendererComponent>(
        mPbrPipeline,
        renderRequiredData.displayName.c_str()
    );
    MFA_ASSERT(renderRequiredData.meshRendererComponent != nullptr);

    entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(1, 0.5, 0.2f));

    EntitySystem::InitEntity(entity);
        
    renderRequiredData.isLoaded = true;
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::destroyModels() {
    // Gpu model
    if (mModelsRenderData.empty() == false) {
        for (auto & group : mModelsRenderData) {
            if (group.isLoaded) {
                RF::DestroyGpuModel(group.gpuModel);
                Importer::FreeModel(group.gpuModel.model);
                group.isLoaded = false;
            }
        }
    }

    // Point light
    MFA_ASSERT(mPointLightModel.valid);
    RF::DestroyGpuModel(mPointLightModel);
    Importer::FreeModel(mPointLightModel.model);
}

//-------------------------------------------------------------------------------------------------

void GLTFMeshViewerScene::updateProjectionBuffer() {
    float projectionData [16];
    mCamera->GetProjection(projectionData);          // TODO It can return
    mPbrPipeline.UpdateCameraProjection(projectionData);
    mPointLightPipeline.UpdateCameraProjection(projectionData);
}

//-------------------------------------------------------------------------------------------------
