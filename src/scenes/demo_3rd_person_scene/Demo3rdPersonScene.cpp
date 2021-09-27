#include "Demo3rdPersonScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/InputManager.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::Demo3rdPersonScene()
    : Scene()
    , mRecordObject([this]()->void { OnUI(); })
{};

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::~Demo3rdPersonScene() = default;

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Init()
{
    // TODO Add directional light!
    {// Error texture
        auto cpuTexture = Importer::CreateErrorTexture();
        mErrorTexture = RF::CreateTexture(cpuTexture);
    }
    {// Sampler
        mSampler = RF::CreateSampler(RT::CreateSamplerParams{});
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
    }
    {// Soldier
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/warcraft_3_alliance_footmanfanmade/scene.gltf").c_str());
        mSoldierGpuModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("Soldier", mSoldierGpuModel);

        {// Playable character
            mSoldierVariant = mPbrPipeline.CreateDrawableVariant("Soldier");

            float position[3]{ 0.0f, 2.0f, -5.0f };
            float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            mSoldierVariant->UpdateTransform(position, eulerAngles, scale);
        }
        {// NPCs
            for (uint32_t i = 0; i < 10; ++i)
            {
                for (uint32_t j = 0; j < 10; ++j) {
                    mNPCs.emplace_back(mPbrPipeline.CreateDrawableVariant("Soldier"));

                    float position[3]{static_cast<float>(i) - 5.0f, 2.0f, static_cast<float>(j) - 4.0f};
                    float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
                    float scale[3]{ 1.0f, 1.0f, 1.0f };
                    mNPCs.back()->UpdateTransform(position, eulerAngles, scale);
                }
            }
        }
    }
    {// Map
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/sponza/sponza.gltf").c_str());
        mMapModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("SponzaMap", mMapModel);
        mMapVariant = mPbrPipeline.CreateDrawableVariant("SponzaMap");

        float position[3]{ 0.4f, 2.0f, -6.0f };
        float eulerAngle[3]{ 180.0f, -90.0f, 0.0f };
        float scale[3]{ 1.0f, 1.0f, 1.0f };
        mMapVariant->UpdateTransform(position, eulerAngle, scale);
    }
    {// Camera
        float eulerAngle[3]{ -15.0f, 0.0f, 0.0f };
        mCamera.Init(mSoldierVariant, 3.0f, eulerAngle);

        updateProjectionBuffer();
    }
    mRecordObject.Enable();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPreRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass)
{
    {// Soldier
        static constexpr float SoldierSpeed = 5.0f;
        auto const inputForwardMove = IM::GetForwardMove();
        auto const inputRightMove = IM::GetRightMove();
        if (inputForwardMove != 0.0f || inputRightMove != 0.0f)
        {
            float position[3]{};
            mSoldierVariant->GetPosition(position);
            float scale[3]{};
            mSoldierVariant->GetScale(scale);
            float targetEulerAngles[3]{};
            mSoldierVariant->GetRotation(targetEulerAngles);

            float cameraEulerAngles[3];
            mCamera.GetRotation(cameraEulerAngles);

            targetEulerAngles[1] = cameraEulerAngles[1];
            float extraAngleValue;
            if (inputRightMove == 1.0f)
            {
                if (inputForwardMove == 1.0f)
                {
                    extraAngleValue = +45.0f;
                }
                else if (inputForwardMove == 0.0f)
                {
                    extraAngleValue = +90.0f;
                }
                else if (inputForwardMove == -1.0f)
                {
                    extraAngleValue = +135.0f;
                }
                else
                {
                    MFA_ASSERT(false);
                }
            }
            else if (inputRightMove == 0.0f)
            {
                if (inputForwardMove == 1.0f)
                {
                    extraAngleValue = 0.0f;
                }
                else if (inputForwardMove == 0.0f)
                {
                    extraAngleValue = 0.0f;
                }
                else if (inputForwardMove == -1.0f)
                {
                    extraAngleValue = +180.0f;
                }
                else
                {
                    MFA_ASSERT(false);
                }
            }
            else if (inputRightMove == -1.0f)
            {
                if (inputForwardMove == 1.0f)
                {
                    extraAngleValue = -45.0f;
                }
                else if (inputForwardMove == 0.0f)
                {
                    extraAngleValue = -90.0f;
                }
                else if (inputForwardMove == -1.0f)
                {
                    extraAngleValue = -135.0f;
                }
                else
                {
                    MFA_ASSERT(false);
                }
            }
            else
            {
                MFA_ASSERT(false);
            }

            targetEulerAngles[1] += extraAngleValue;

            float currentEulerAngles[3];
            mSoldierVariant->GetRotation(currentEulerAngles);

            auto const targetQuat = Matrix::GlmToQuat(currentEulerAngles[0], targetEulerAngles[1], currentEulerAngles[2]);

            auto const currentQuat = Matrix::GlmToQuat(currentEulerAngles[0], currentEulerAngles[1], currentEulerAngles[2]);

            auto const nextQuat = glm::slerp(currentQuat, targetQuat, 10.0f * deltaTimeInSec);
            auto nextAnglesVec3 = Matrix::GlmToEulerAngles(nextQuat);

            float nextAngles[3]{ nextAnglesVec3[0], nextAnglesVec3[1], nextAnglesVec3[2] };

            if (std::fabs(nextAngles[2]) >= 90)
            {
                nextAngles[0] += 180.f;
                nextAngles[1] = 180.f - nextAngles[1];
                nextAngles[2] += 180.f;
            }

            auto rotationMatrix = glm::identity<glm::mat4>();
            Matrix::GlmRotate(rotationMatrix, nextAngles);

            glm::vec4 forwardDirection(
                CameraBase::ForwardVector[0],
                CameraBase::ForwardVector[1],
                CameraBase::ForwardVector[2],
                CameraBase::ForwardVector[3]
            );
            forwardDirection = forwardDirection * rotationMatrix;
            forwardDirection = glm::normalize(forwardDirection);
            forwardDirection *= 1 * deltaTimeInSec * SoldierSpeed;

            position[0] += forwardDirection[0];
            position[1] += forwardDirection[1];
            position[2] += forwardDirection[2];

            mSoldierVariant->UpdateTransform(
                position,
                nextAngles,
                scale
            );

            mSoldierVariant->SetActiveAnimation("SwordAndShieldRun", { .transitionDuration = 0.3f });

        }
        else
        {

            //mSoldierVariant->SetActiveAnimation("SwordAndShieldIdle");
            mSoldierVariant->SetActiveAnimation("Idle", { .transitionDuration = 0.3f });

        }
    }
    // TODO We should listen for player input and move character here
    mCamera.OnUpdate(deltaTimeInSec);

    {// Read camera View to update pipeline buffer
        float viewData[16];
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

void Demo3rdPersonScene::OnRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass)
{
    mPointLightPipeline.Render(drawPass, deltaTimeInSec);
    mPbrPipeline.Render(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPostRender(float const deltaTimeInSec, MFA::RT::DrawPass & drawPass)
{
    mPointLightPipeline.PostRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PostRender(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Shutdown()
{
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
    {// Point light
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

void Demo3rdPersonScene::OnResize()
{
    mCamera.OnResize();
    updateProjectionBuffer();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::updateProjectionBuffer()
{
    float projectionData[16];
    mCamera.GetProjection(projectionData);
    mPbrPipeline.UpdateCameraProjection(projectionData);
    mPointLightPipeline.UpdateCameraProjection(projectionData);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnUI()
{
    mCamera.OnUI();
}

//-------------------------------------------------------------------------------------------------
