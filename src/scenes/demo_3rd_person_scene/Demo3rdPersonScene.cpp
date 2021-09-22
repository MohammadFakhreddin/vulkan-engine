#include "Demo3rdPersonScene.hpp"

#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/ShapeGenerator.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::Demo3rdPersonScene()
    : Scene()
    , mRecordObject([this]()->void {OnUI();})
{};

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::~Demo3rdPersonScene() = default;

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Init() {
    // TODO Add directional light!
    {// Error texture
        auto cpuTexture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpuTexture);
    }
    {// Sampler
        mSampler = RF::CreateSampler(RT::CreateSamplerParams {});
    }
    {// Pbr pipeline
        mPbrPipeline.Init(&mSampler, &mErrorTexture, Z_NEAR, Z_FAR);

        mPbrPipeline.UpdateLightPosition(mLightPosition);
        mPbrPipeline.UpdateLightColor(mLightColor);
    }
    {// PointLight

        mPointLightPipeline.Init();

        auto cpuModel = MFA::ShapeGenerator::Sphere(0.1f);

        mPointLightModel = RF::CreateGpuModel(cpuModel);
        mPointLightPipeline.CreateDrawableEssence("Sphere", mPointLightModel);
        mPointLightVariant = mPointLightPipeline.CreateDrawableVariant("Sphere");
        
        mPointLightVariant->UpdatePosition(mLightPosition);
        mPointLightPipeline.UpdateLightColor(mPointLightVariant, mLightColor);

        mPointLightVariant->SetActive(false);
    }
    {// Soldier
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/warcraft_3_alliance_footmanfanmade/scene.gltf").c_str());
        mSoldierGpuModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("Soldier", mSoldierGpuModel);
        mSoldierVariant = mPbrPipeline.CreateDrawableVariant("Soldier");

        float position[3] {0.0f, 2.0f, 0.0f};
        float eulerAngles[3] {0.0f, 180.0f, -180.0f};
        float scale[3] {1.0f, 1.0f, 1.0f};
        mSoldierVariant->UpdateTransform(position, eulerAngles, scale);
    }
    {// Map
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/sponza/sponza.gltf").c_str());
        mMapModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("SponzaMap", mMapModel);
        mMapVariant = mPbrPipeline.CreateDrawableVariant("SponzaMap");

        float position[3] {0.4f, 2.0f, -6.0f};
        float eulerAngle[3] {180.0f, -90.0f, 0.0f};
        float scale[3] {1.0f, 1.0f, 1.0f};
        mMapVariant->UpdateTransform(position, eulerAngle, scale);
    }
    {// Camera
        float distance[3] {0.0f, 0.0f, 5.0f};
        float eulerAngle[3] {0.0f, 0.0f, 0.0f};
        //mCamera.Init(mSoldierVariant, distance, eulerAngle);
        mCamera.Init();

        float position[3] {0.104f, 1.286f, 4.952f};
        float eulerAngles[3] {-12.0f, 3.0f, 0.0f};
        mCamera.ForcePosition(position);
        mCamera.ForceRotation(eulerAngles);

        updateProjectionBuffer();
    }
    mRecordObject.Enable();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPreRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass) {
    // TODO We should listen for player input and move character here
    mCamera.OnUpdate(deltaTimeInSec);

    {// Read camera View to update pipeline buffer
        float viewData [16];
        mCamera.GetTransform(viewData);
        mPbrPipeline.UpdateCameraView(viewData);
        mPointLightPipeline.UpdateCameraView(viewData);

        float cameraPosition[3];
        mCamera.GetPosition(cameraPosition);
        mPbrPipeline.UpdateCameraPosition(cameraPosition);

    }
    mPointLightPipeline.PreRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PreRender(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass) {
    mPointLightPipeline.Render(drawPass, deltaTimeInSec);
    mPbrPipeline.Render(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPostRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass) {
    mPointLightPipeline.PostRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PostRender(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Shutdown() {
    mRecordObject.Disable();
    {// Soldier
        RF::DestroyGpuModel(mSoldierGpuModel);
        Importer::FreeModel(mSoldierGpuModel.model);
    }
    {// Map
        // TODO: I think we can destroy cpu model after uploading to gpu
        RF::DestroyGpuModel(mMapModel);
        Importer::FreeModel(mMapModel.model);
    }
    {// PointLight
        RF::DestroyGpuModel(mPointLightModel);
        Importer::FreeModel(mPointLightModel.model);
    }
    {// Pbr pipeline
        mPbrPipeline.Shutdown();
    }
    {
        mPointLightPipeline.Shutdown();   
    }
    {// Sampler
        RF::DestroySampler(mSampler);
    }
    {// Error texture
        RF::DestroyTexture(mErrorTexture);        
    }
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnResize() {
    mCamera.OnResize();
    updateProjectionBuffer();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::updateProjectionBuffer() {
    float projectionData [16];
    mCamera.GetProjection(projectionData);
    mPbrPipeline.UpdateCameraProjection(projectionData);
    mPointLightPipeline.UpdateCameraProjection(projectionData);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnUI() {
    //mCamera.OnUI();
}

//-------------------------------------------------------------------------------------------------
