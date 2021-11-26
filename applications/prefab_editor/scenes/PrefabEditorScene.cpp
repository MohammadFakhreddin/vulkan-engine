#include "PrefabEditorScene.hpp"

#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/ui_system/UISystem.hpp"

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
    mPbrPipeline.Init(&mSampler, &mErrorTexture, Z_NEAR, Z_FAR);
    
    {// Debug renderer pipeline
        mDebugRenderPipeline.Init();

        auto sphereCpuModel = ShapeGenerator::Sphere();
        mSphereModel = RF::CreateGpuModel(sphereCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Sphere", mSphereModel);

        auto cubeCpuModel = ShapeGenerator::Cube();
        mCubeModel = RF::CreateGpuModel(cubeCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Cube", mCubeModel);
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
    float deltaTimeInSec,
    RT::CommandRecordState & recordState
)
{
    Scene::OnRender(deltaTimeInSec, recordState);

    mDebugRenderPipeline.Render(recordState, deltaTimeInSec);
    mPbrPipeline.Render(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void PrefabEditorScene::OnPostRender(
    float deltaTimeInSec,
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

void PrefabEditorScene::onUI() const
{
    // TODO Use this link to open the file picker dialog
    //https://docs.microsoft.com/en-us/windows/win32/learnwin32/example--the-open-dialog-box
}

//-------------------------------------------------------------------------------------------------
