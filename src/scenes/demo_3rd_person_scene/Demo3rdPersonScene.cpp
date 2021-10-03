#include "Demo3rdPersonScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/InputManager.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::Demo3rdPersonScene()
    : Scene()
{
}

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::~Demo3rdPersonScene() = default;

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Init()
{
    Scene::Init();
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

        auto * entity = EntitySystem::CreateEntity("PointLight", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto * colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent != nullptr);
        colorComponent->SetColor(mLightColor);

        auto * transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdatePosition(mLightPosition);

        auto * meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPointLightPipeline, "Sphere");
        MFA_ASSERT(meshRendererComponent != nullptr);

        entity->AddComponent<SphereBoundingVolumeComponent>(0.1f);
        
        EntitySystem::InitEntity(entity);
    }
    {// Soldier
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/warcraft_3_alliance_footmanfanmade/scene.gltf").c_str());
        mSoldierGpuModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("Soldier", mSoldierGpuModel);

        {// Playable character
            auto * entity = EntitySystem::CreateEntity("Playable soldier", GetRootEntity());
            MFA_ASSERT(entity != nullptr);

            mPlayerTransform = entity->AddComponent<TransformComponent>();
            MFA_ASSERT(mPlayerTransform != nullptr);

            float position[3]{ 0.0f, 2.0f, -5.0f };
            float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            mPlayerTransform->UpdateTransform(position, eulerAngles, scale);

            mPlayerMeshRenderer = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "Soldier");
            MFA_ASSERT(mPlayerMeshRenderer != nullptr);

            entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(1, 0.5, 0.2f));

            mThirdPersonCamera = entity->AddComponent<ThirdPersonCameraComponent>(
                FOV,
                Z_FAR,
                Z_NEAR
            );
            MFA_ASSERT(mThirdPersonCamera != nullptr);
            float eulerAngle[3]{ -15.0f, 0.0f, 0.0f };
            mThirdPersonCamera->SetDistanceAndRotation(3.0f, eulerAngle);
    
            SetActiveCamera(mThirdPersonCamera);

            EntitySystem::InitEntity(entity);
        }
        {// NPCs
            for (uint32_t i = 0; i < 10; ++i)
            {
                for (uint32_t j = 0; j < 10; ++j) {
                    auto * entity = EntitySystem::CreateEntity("Random soldier", GetRootEntity());
                    MFA_ASSERT(entity != nullptr);

                    auto * transformComponent = entity->AddComponent<TransformComponent>();
                    MFA_ASSERT(transformComponent != nullptr);

                    float position[3]{static_cast<float>(i) - 5.0f, 2.0f, static_cast<float>(j) - 4.0f};
                    float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
                    float scale[3]{ 1.0f, 1.0f, 1.0f };
                    transformComponent->UpdateTransform(position, eulerAngles, scale);

                    auto * meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "Soldier");
                    MFA_ASSERT(meshRendererComponent != nullptr);

                    entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(1, 0.5, 0.2f));

                    EntitySystem::InitEntity(entity);
                }
            }
        }
    }
    {// Map
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/sponza/sponza.gltf").c_str());
        mMapModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("SponzaMap", mMapModel);

        auto * entity = EntitySystem::CreateEntity("Sponza scene", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto * transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);

        float position[3]{ 0.4f, 2.0f, -6.0f };
        float eulerAngle[3]{ 180.0f, -90.0f, 0.0f };
        float scale[3]{ 1.0f, 1.0f, 1.0f };
        transformComponent->UpdateTransform(position, eulerAngle, scale);

        auto * meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "SponzaMap");
        MFA_ASSERT(meshRendererComponent != nullptr);
        // TODO Generate default AABB from vertices then let use modify it
        entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(10, 10, 10));

        EntitySystem::InitEntity(entity);
    }
    mUIRecordId = UI::Register([this]()->void {onUI();});
    updateProjectionBuffer();
    
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
            mPlayerTransform->GetPosition(position);
            float scale[3]{};
            mPlayerTransform->GetScale(scale);
            float targetEulerAngles[3]{};
            mPlayerTransform->GetRotation(targetEulerAngles);

            float cameraEulerAngles[3];
            mThirdPersonCamera->GetRotation(cameraEulerAngles);

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
            mPlayerTransform->GetRotation(currentEulerAngles);

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
                CameraComponent::ForwardVector[0],
                CameraComponent::ForwardVector[1],
                CameraComponent::ForwardVector[2],
                CameraComponent::ForwardVector[3]
            );
            forwardDirection = forwardDirection * rotationMatrix;
            forwardDirection = glm::normalize(forwardDirection);
            forwardDirection *= 1 * deltaTimeInSec * SoldierSpeed;

            position[0] += forwardDirection[0];
            position[1] += forwardDirection[1];
            position[2] += forwardDirection[2];

            mPlayerTransform->UpdateTransform(
                position,
                nextAngles,
                scale
            );
            // TODO What should we do for animations ?
            mPlayerMeshRenderer->GetVariant()->SetActiveAnimation("SwordAndShieldRun", { .transitionDuration = 0.3f });

        }
        else
        {
            //mSoldierVariant->SetActiveAnimation("SwordAndShieldIdle");
            mPlayerMeshRenderer->GetVariant()->SetActiveAnimation("Idle", { .transitionDuration = 0.3f });

        }
    }

    // TODO Pipelines should get info of active camera by themselves
    {// Read camera View to update pipeline buffer
        auto * activeCamera = GetActiveCamera();

        float viewData[16];
        activeCamera->GetTransform(viewData);
        mPbrPipeline.UpdateCameraView(viewData);
        mPointLightPipeline.UpdateCameraView(viewData);

        float cameraPosition[3];
        activeCamera->GetPosition(cameraPosition);
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
    Scene::Shutdown();

    UI::UnRegister(mUIRecordId);

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
    auto * camera = GetActiveCamera();
    MFA_ASSERT(camera != nullptr);
    camera->OnResize();
    updateProjectionBuffer();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::updateProjectionBuffer()
{
    auto * camera = GetActiveCamera();
    MFA_ASSERT(camera != nullptr);
    float projectionData[16];
    camera->GetProjection(projectionData);
    mPbrPipeline.UpdateCameraProjection(projectionData);
    mPointLightPipeline.UpdateCameraProjection(projectionData);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::onUI() const
{
    auto * camera = GetActiveCamera();
    MFA_ASSERT(camera != nullptr);
    camera->OnUI();
}

//-------------------------------------------------------------------------------------------------
