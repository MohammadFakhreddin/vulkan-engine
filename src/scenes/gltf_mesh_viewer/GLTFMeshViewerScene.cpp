#include "GLTFMeshViewerScene.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/DrawableObject.hpp"
#include "tools/Importer.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/BedrockPath.hpp"

namespace RF = MFA::RenderFrontend;
namespace Importer = MFA::Importer;
namespace UI = MFA::UISystem;
namespace Path = MFA::Path;

GLTFMeshViewerScene::GLTFMeshViewerScene()
    : Scene()
    , mRecordObject([this]()->void {OnUI();})
{}

void GLTFMeshViewerScene::Init() {
    // TODO Out of pool memory on MacOs, We need to only keep a few of recent objects data active.
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
            params.displayName = "Car";
            Path::Asset("models/free_zuk_3d_model/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-19.0f, -32.0f, 177.0f});
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -2.0f});
            mModelsRenderData.emplace_back(params);
        }
        {
            ModelRenderRequiredData params {};
            params.displayName = "Gunship";
            Path::Asset("models/gunship/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {160.0f, 176.0f, 0.0f});
            params.initialParams.model.scale = 0.008f;
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -33.745f});
            MFA::Copy<3>(params.initialParams.light.position, {-2.0f, -2.0f, -29.0f});
            MFA::Copy<3>(params.initialParams.camera.position, {2.5f, 5.3f, 21.8f});
            MFA::Copy<3>(params.initialParams.camera.eulerAngles, {-22.110f, -13.200f, 0.0f});
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
            params.displayName = "Mandalorian2";
            Path::Asset("models/mandalorian__the_fortnite_season_6_skin_updated/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {180.0f, 180.0f, 0.0f});
            params.initialParams.model.scale = 0.0005f;
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 4.5f, -10.0f});
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
        {
            ModelRenderRequiredData params {};
            params.displayName = "Warhammer tank";
            Path::Asset("models/warhammer_40k_predator_dark_millennium/scene.gltf", params.address);
            MFA::Copy<3>(params.initialParams.model.rotationEulerAngle, {-45.0f, -45.0f, 180.0f});
            params.initialParams.model.scale = 0.5f;
            MFA::Copy<3>(params.initialParams.model.translate, {0.0f, 0.0f, -16.0f});
            MFA::Copy<3>(params.initialParams.light.position, {1.5f, 0.0f, -8.0f});
            mModelsRenderData.emplace_back(params);
        }
    }

    // TODO We should use gltf sampler info here
    mSamplerGroup = RF::CreateSampler();
    
    mPointLightPipeline.init();

    mCamera.init();
    mCamera.EnableUI("", &mIsCameraWindowVisible);

    mPbrPipeline.Init(&mSamplerGroup, &mErrorTexture, Z_NEAR, Z_FAR);

    // Updating perspective mat once for entire application
    // Perspective
    updateProjectionBuffer();

    {// Point light
        auto cpuModel = MFA::ShapeGenerator::Sphere(0.1f);

        for (uint32_t i = 0; i < cpuModel.mesh.GetSubMeshCount(); ++i) {
            for (auto & primitive : cpuModel.mesh.GetSubMeshByIndex(i).primitives) {
                primitive.baseColorFactor[0] = 1.0f;
                primitive.baseColorFactor[1] = 1.0f;
                primitive.baseColorFactor[2] = 1.0f;
            }
        }

        mGpuPointLight = RF::CreateGpuModel(cpuModel);
        mPointLightObjectId = mPointLightPipeline.AddGpuModel(mGpuPointLight);
    }

    mRecordObject.Enable();
}

void GLTFMeshViewerScene::OnUpdate(float deltaTimeInSec, MFA::RenderFrontend::DrawPass & drawPass) {
    auto & selectedModel = mModelsRenderData[mSelectedModelIndex];
    if (selectedModel.isLoaded == false) {
        createModel(selectedModel);
    }
    if (mPreviousModelSelectedIndex != mSelectedModelIndex) {
        {// Enabling ui for current model
            auto * selectedDrawable = mPbrPipeline.GetDrawableById(mModelsRenderData[mSelectedModelIndex].drawableObjectId);
            MFA_ASSERT(selectedDrawable != nullptr);
            selectedDrawable->EnableUI("Active model", &mIsDrawableObjectWindowVisible);
        }
        if (mPreviousModelSelectedIndex >= 0) {// Disabling ui for previous model
            auto * previousDrawable = mPbrPipeline.GetDrawableById(mModelsRenderData[mPreviousModelSelectedIndex].drawableObjectId);
            MFA_ASSERT(previousDrawable != nullptr);
            previousDrawable->DisableUI();
        }

        mPreviousModelSelectedIndex = mSelectedModelIndex;
        // Model
        MFA::Copy<3>(m_model_rotation, selectedModel.initialParams.model.rotationEulerAngle);
        MFA::Copy<3>(m_model_position, selectedModel.initialParams.model.translate);
        m_model_scale = selectedModel.initialParams.model.scale;
        MFA::Copy<3>(mModelTranslateMin, selectedModel.initialParams.model.translateMin);
        MFA::Copy<3>(mModelTranslateMax, selectedModel.initialParams.model.translateMax);
        
        // Light
        MFA::Copy<3>(mLightPosition, selectedModel.initialParams.light.position);
        MFA::Copy<3>(mLightColor, selectedModel.initialParams.light.color);
        MFA::Copy<3>(mLightTranslateMin, selectedModel.initialParams.light.translateMin);
        MFA::Copy<3>(mLightTranslateMax, selectedModel.initialParams.light.translateMax);
        
        mCamera.forcePosition(selectedModel.initialParams.camera.position);
        mCamera.forceRotation(selectedModel.initialParams.camera.eulerAngles);
    }

    mPbrPipeline.Update(drawPass, deltaTimeInSec, 1, &selectedModel.drawableObjectId);
}

void GLTFMeshViewerScene::OnDraw(float const deltaTimeInSec, RF::DrawPass & drawPass) {
    MFA_ASSERT(mSelectedModelIndex >= 0 && mSelectedModelIndex < mModelsRenderData.size());

    mCamera.onNewFrame(deltaTimeInSec);

    auto & selectedModel = mModelsRenderData[mSelectedModelIndex];
    {// Updating Transform buffer
        // Rotation
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            m_model_rotation[0],
            m_model_rotation[1],
            m_model_rotation[2]
        );

        // Scale
        MFA::Matrix4X4Float scaleMat {};
        MFA::Matrix4X4Float::AssignScale(scaleMat, m_model_scale);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            m_model_position[0],
            m_model_position[1],
            m_model_position[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::Identity(transformMat);
        transformMat.multiply(translationMat);
        transformMat.multiply(rotationMat);
        transformMat.multiply(scaleMat);
        
        transformMat.copy(mPbrMVPData.model);

        mCamera.getTransform(mPbrMVPData.view);

        mPbrPipeline.UpdateViewProjectionBuffer(
            selectedModel.drawableObjectId,
            mPbrMVPData
        );
    }
    {// LightViewBuffer
        //auto lightPosition = glm::vec4(
        //    mLightPosition[0], 
        //    mLightPosition[1], 
        //    mLightPosition[2], 
        //    1.0f
        //);

        //glm::mat4x4 transformMatrix;
        //MFA::Matrix4X4Float::ConvertMatrixToGlm(mCamera.getTransform(), transformMatrix);

        //lightPosition = transformMatrix * lightPosition;

        //float transformedLightPosition[4] = {lightPosition[0], lightPosition[1], lightPosition[2]};
        mPbrPipeline.UpdateLightPosition(mLightPosition);
        float cameraPosition[3];
        mCamera.GetPosition(cameraPosition);
        mPbrPipeline.UpdateCameraPosition(cameraPosition);
        mPbrPipeline.UpdateLightColor(mLightColor);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            mLightPosition[0],
            mLightPosition[1],
            mLightPosition[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::Identity(transformMat);
        transformMat.multiply(translationMat);

        transformMat.copy(mPointLightMVPData.model);
        mCamera.getTransform(mPointLightMVPData.view);
        
        mPointLightPipeline.updateViewProjectionBuffer(mPointLightObjectId, mPointLightMVPData);

        MFA::PointLightPipeline::PrimitiveInfo lightPrimitiveInfo {};
        MFA::Copy<3>(lightPrimitiveInfo.baseColorFactor, mLightColor);
        lightPrimitiveInfo.baseColorFactor[3] = 1.0f;
        mPointLightPipeline.updatePrimitiveInfo(mPointLightObjectId, lightPrimitiveInfo);
    }
    // TODO Pipeline should be able to share buffers such as projection buffer to enable us to update them once
    mPbrPipeline.Render(drawPass, deltaTimeInSec, 1, &selectedModel.drawableObjectId);
    if (mIsLightVisible) {
        mPointLightPipeline.Render(drawPass, deltaTimeInSec, 1, &mPointLightObjectId);
    }
}

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
    UI::Checkbox("Active model", &mIsDrawableObjectWindowVisible);
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
        UI::SliderFloat("XDegree", &m_model_rotation[0], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDegree", &m_model_rotation[1], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDegree", &m_model_rotation[2], -360.0f, 360.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("Scale", &m_model_scale, 0.0f, 1.0f);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("XDistance", &m_model_position[0], mModelTranslateMin[0], mModelTranslateMax[0]);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("YDistance", &m_model_position[1], mModelTranslateMin[1], mModelTranslateMax[1]);
        UI::Spacing();
        UI::SetNextItemWidth(ItemWidth);
        UI::SliderFloat("ZDistance", &m_model_position[2], mModelTranslateMin[2], mModelTranslateMax[2]);
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
   
    // TODO Node tree
}

void GLTFMeshViewerScene::Shutdown() {
    {// Disabling ui for current model
        auto * selectedDrawable = mPbrPipeline.GetDrawableById(mModelsRenderData[mSelectedModelIndex].drawableObjectId);
        MFA_ASSERT(selectedDrawable != nullptr);
        selectedDrawable->DisableUI();
    }
    mCamera.DisableUI();
    mRecordObject.Disable();
    mPbrPipeline.Shutdown();
    mPointLightPipeline.shutdown();
    RF::DestroySampler(mSamplerGroup);
    destroyModels();
    RF::DestroyTexture(mErrorTexture);
    Importer::FreeTexture(mErrorTexture.cpuTexture());
}

void GLTFMeshViewerScene::OnResize() {
    mCamera.onResize();
    updateProjectionBuffer();
}

void GLTFMeshViewerScene::createModel(ModelRenderRequiredData & renderRequiredData) {
    auto cpuModel = Importer::ImportGLTF(renderRequiredData.address.c_str());
    renderRequiredData.gpuModel = RF::CreateGpuModel(cpuModel);
    renderRequiredData.drawableObjectId = mPbrPipeline.AddGpuModel(renderRequiredData.gpuModel);
    renderRequiredData.isLoaded = true;
}

void GLTFMeshViewerScene::destroyModels() {
    // Gpu model
    if (mModelsRenderData.empty() == false) {
        for (auto & group : mModelsRenderData) {
            if (group.isLoaded) {
                RF::DestroyGpuModel(group.gpuModel);
                Importer::FreeModel(&group.gpuModel.model);
                group.isLoaded = false;
            }
        }
    }

    // Point light
    MFA_ASSERT(mGpuPointLight.valid);
    RF::DestroyGpuModel(mGpuPointLight);
    Importer::FreeModel(&mGpuPointLight.model);
}

void GLTFMeshViewerScene::updateProjectionBuffer() {
    // PBR
    mCamera.getProjection(mPbrMVPData.projection);
    // PointLight
    mCamera.getProjection(mPointLightMVPData.projection);
}