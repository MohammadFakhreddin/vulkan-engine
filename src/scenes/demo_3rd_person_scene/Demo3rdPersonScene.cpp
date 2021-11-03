#include "Demo3rdPersonScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/InputManager.hpp"
#include "engine/camera/ThirdPersonCameraComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/SphereBoundingVolumeComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/PointLightComponent.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::Demo3rdPersonScene()
    : Scene()
{}

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::~Demo3rdPersonScene() = default;

//-------------------------------------------------------------------------------------------------
// TODO Make pipelines global that start at beginning of application or not ?
void Demo3rdPersonScene::Init()
{
    Scene::Init();
    // TODO Add directional light!
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

        auto sphereCpuModel = MFA::ShapeGenerator::Sphere();
        mSphereModel = RF::CreateGpuModel(sphereCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Sphere", mSphereModel);

        auto cubeCpuModel = ShapeGenerator::Cube();
        mCubeModel = RF::CreateGpuModel(cubeCpuModel);
        mDebugRenderPipeline.CreateDrawableEssence("Cube", mCubeModel);
    }

    // TODO We need prefab system!
    {// Soldier
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/warcraft_3_alliance_footmanfanmade/scene.gltf").c_str());
        mSoldierGpuModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("Soldier", mSoldierGpuModel);
    }
    {// Playable character
        auto * entity = EntitySystem::CreateEntity("Playable soldier", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        mPlayerTransform = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(mPlayerTransform.expired() == false);
        if (auto const ptr = mPlayerTransform.lock())
        {
            float position[3]{ 0.0f, 2.0f, -5.0f };
            float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            ptr->UpdateTransform(position, eulerAngles, scale);
        }

        mPlayerMeshRenderer = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "Soldier");
        MFA_ASSERT(mPlayerMeshRenderer.expired() == false);
        mPlayerMeshRenderer.lock()->SetActive(true);

        entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        auto colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent.expired() == false);
        colorComponent.lock()->SetColor(glm::vec3{ 1.0f, 0.0f, 0.0f });

        auto debugRenderComponent = entity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline);
        MFA_ASSERT(debugRenderComponent.expired() == false);
        debugRenderComponent.lock()->SetActive(false);

        mThirdPersonCamera = entity->AddComponent<ThirdPersonCameraComponent>(
            FOV,
            Z_NEAR,
            Z_FAR
        );
        MFA_ASSERT(mThirdPersonCamera.expired() == false);
        float eulerAngle[3]{ -15.0f, 0.0f, 0.0f };
        mThirdPersonCamera.lock()->SetDistanceAndRotation(3.0f, eulerAngle);

        SetActiveCamera(mThirdPersonCamera);

        EntitySystem::InitEntity(entity);
    }
    {// NPCs
        for (uint32_t i = 0; i < 10/*33*/; ++i)
        {
            for (uint32_t j = 0; j < 10/*33*/; ++j)
            {
                auto * entity = EntitySystem::CreateEntity("Random soldier", GetRootEntity());
                MFA_ASSERT(entity != nullptr);

                auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
                MFA_ASSERT(transformComponent != nullptr);
                float position[3]{ static_cast<float>(i) - 15.0f, 2.0f, static_cast<float>(j) - 30.0f };
                float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
                float scale[3]{ 1.0f, 1.0f, 1.0f };
                transformComponent->UpdateTransform(position, eulerAngles, scale);
                
                auto const meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "Soldier").lock();
                MFA_ASSERT(meshRendererComponent != nullptr);
                meshRendererComponent->GetVariant().lock()->SetActiveAnimation(
                    "SwordAndShieldIdle",
                    { .startTimeOffsetInSec = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 10 }
                );
                meshRendererComponent->SetActive(true);
                
                entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

                auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
                MFA_ASSERT(colorComponent != nullptr);
                colorComponent->SetColor(glm::vec3{ 0.0f, 0.0f, 1.0f });
                
                auto const debugRenderComponent = entity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline).lock();
                MFA_ASSERT(debugRenderComponent != nullptr);
                debugRenderComponent->SetActive(false);
                
                EntitySystem::InitEntity(entity);

                entity->SetActive(false);
            }
        }
    }

    std::weak_ptr<DrawableVariant> mapVariant {};

    {// Map
        auto cpuModel = Importer::ImportGLTF(Path::Asset("models/sponza/sponza.gltf").c_str());
        mMapModel = RF::CreateGpuModel(cpuModel);
        mPbrPipeline.CreateDrawableEssence("SponzaMap", mMapModel);

        auto * entity = EntitySystem::CreateEntity("Sponza scene", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent.expired() == false);

        if (auto const ptr = transformComponent.lock())
        {
            float position[3]{ 0.4f, 2.0f, -6.0f };
            float eulerAngle[3]{ 180.0f, -90.0f, 0.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            ptr->UpdateTransform(position, eulerAngle, scale);
        }

        auto const meshRendererComponent = entity->AddComponent<MeshRendererComponent>(mPbrPipeline, "SponzaMap").lock();
        mapVariant = meshRendererComponent->GetVariant();

        entity->AddComponent<AxisAlignedBoundingBoxComponent>(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(15.0f, 6.0f, 9.0f));

        auto const debugRenderComponent = entity->AddComponent<BoundingVolumeRendererComponent>(mDebugRenderPipeline);
        if (auto const ptr = debugRenderComponent.lock())
        {
            ptr->SetActive(false);
        }

        entity->AddComponent<ColorComponent>(glm::vec3(0.0f, 0.0f, 1.0f));

        entity->SetActive(true);

        EntitySystem::InitEntity(entity);
    }

    {// PointLight

        auto * entity = EntitySystem::CreateEntity("PointLight", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
        MFA_ASSERT(colorComponent != nullptr);
        colorComponent->SetColor(mLightColor);
        
        auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdatePosition(mLightPosition);
        transformComponent->UpdateScale(glm::vec3(0.1f, 0.1f, 0.1f));
        
        entity->AddComponent<MeshRendererComponent>(mDebugRenderPipeline, "Sphere");
        
        entity->AddComponent<SphereBoundingVolumeComponent>(0.1f);

        // TODO Maybe we can read radius from transform component instead
        entity->AddComponent<PointLightComponent>(1.0f, 100.0f, Z_NEAR, Z_FAR, mapVariant);

        EntitySystem::InitEntity(entity);
    }

    mUIRecordId = UI::Register([this]()->void { onUI(); });
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPreRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & drawPass)
{
    Scene::OnPreRender(deltaTimeInSec, drawPass);

    mDebugRenderPipeline.PreRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PreRender(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & drawPass)
{
    Scene::OnRender(deltaTimeInSec, drawPass);

    mDebugRenderPipeline.Render(drawPass, deltaTimeInSec);
    mPbrPipeline.Render(drawPass, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::OnPostRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & drawPass)
{
    Scene::OnPostRender(deltaTimeInSec, drawPass);

    mDebugRenderPipeline.PostRender(drawPass, deltaTimeInSec);
    mPbrPipeline.PostRender(drawPass, deltaTimeInSec);

    if (auto const playerTransformPtr = mPlayerTransform.lock())
    {// Soldier
        static constexpr float SoldierSpeed = 4.0f;
        auto const inputForwardMove = IM::GetForwardMove();
        auto const inputRightMove = IM::GetRightMove();
        if (inputForwardMove != 0.0f || inputRightMove != 0.0f)
        {
            float position[3]{};
            playerTransformPtr->GetPosition(position);
            float scale[3]{};
            playerTransformPtr->GetScale(scale);
            float targetEulerAngles[3]{};
            playerTransformPtr->GetRotation(targetEulerAngles);

            float cameraEulerAngles[3];
            MFA_ASSERT(mThirdPersonCamera.expired() == false);
            mThirdPersonCamera.lock()->GetRotation(cameraEulerAngles);

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
            playerTransformPtr->GetRotation(currentEulerAngles);

            auto const targetQuat = Matrix::ToQuat(currentEulerAngles[0], targetEulerAngles[1], currentEulerAngles[2]);

            auto const currentQuat = Matrix::ToQuat(currentEulerAngles[0], currentEulerAngles[1], currentEulerAngles[2]);

            auto const nextQuat = glm::slerp(currentQuat, targetQuat, 10.0f * deltaTimeInSec);
            auto nextAnglesVec3 = Matrix::ToEulerAngles(nextQuat);

            float nextAngles[3]{ nextAnglesVec3[0], nextAnglesVec3[1], nextAnglesVec3[2] };

            if (std::fabs(nextAngles[2]) >= 90)
            {
                nextAngles[0] += 180.f;
                nextAngles[1] = 180.f - nextAngles[1];
                nextAngles[2] += 180.f;
            }

            auto rotationMatrix = glm::identity<glm::mat4>();
            Matrix::Rotate(rotationMatrix, nextAngles);

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

            playerTransformPtr->UpdateTransform(
                position,
                nextAngles,
                scale
            );
            // TODO What should we do for animations ?
            if (auto const meshRendererPtr = mPlayerMeshRenderer.lock())
            {
                if (auto variant = meshRendererPtr->GetVariant().lock())
                {
                    variant->SetActiveAnimation("SwordAndShieldRun", { .transitionDuration = 0.3f });
                }
            }
        }
        else
        {
            if (auto const meshRendererPtr = mPlayerMeshRenderer.lock())
            {
                if (auto variant = meshRendererPtr->GetVariant().lock())
                {
                    //mSoldierVariant->SetActiveAnimation("SwordAndShieldIdle");
                    variant->SetActiveAnimation("Idle", { .transitionDuration = 0.3f });
                }
            }
        }
    }
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
    {// Sphere
        RF::DestroyGpuModel(mSphereModel);
        Importer::FreeModel(mSphereModel.model);
    }
    {// Cube
        RF::DestroyGpuModel(mCubeModel);
        Importer::FreeModel(mCubeModel.model);
    }
    {// Pbr pipeline
        mPbrPipeline.Shutdown();
    }
    {// Point light
        mDebugRenderPipeline.Shutdown();
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
    mPbrPipeline.OnResize();
    mDebugRenderPipeline.OnResize();
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::onUI() const
{
}

//-------------------------------------------------------------------------------------------------
