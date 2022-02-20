#include "Demo3rdPersonScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockPath.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/InputManager.hpp"
#include "engine/camera/ThirdPersonCameraComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/entity_system/components/DirectionalLightComponent.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "tools/PrefabFileStorage.hpp"
#include "engine/render_system/pipelines/particle/FireEssence.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

namespace MFA
{
    class BoundingVolumeRendererComponent;
}

static constexpr float FireRadius = 0.7f;

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::Demo3rdPersonScene()
    : Scene()
{}

//-------------------------------------------------------------------------------------------------

Demo3rdPersonScene::~Demo3rdPersonScene() = default;

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Init()
{
    Scene::Init();

    Prefab soldierPrefab {EntitySystem::CreateEntity("SolderPrefab", nullptr)};
    Prefab sponzaPrefab {EntitySystem::CreateEntity("SponzaPrefab", nullptr)};
    
    PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams {
        .fileAddress = Path::ForReadWrite("prefabs/soldier.json"),
        .prefab = &soldierPrefab
    });

    PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams {
        .fileAddress = Path::ForReadWrite("prefabs/sponza3.json"),
        .prefab = &sponzaPrefab
    });

    {// Playable soldier
        auto * entity = soldierPrefab.Clone(GetRootEntity(), Prefab::CloneEntityOptions {.name = "Playable soldier"});
        {// Transform
            float position[3]{ 0.0f, 2.0f, -5.0f };
            float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            mPlayerTransform = entity->GetComponent<TransformComponent>();
            mPlayerTransform.lock()->UpdateTransform(
                position,
                eulerAngles,
                scale
            );
        }
        {// Camera
            auto const thirdPersonCamera = entity->AddComponent<ThirdPersonCameraComponent>(FOV, Z_NEAR, Z_FAR).lock();
            float eulerAngle[3]{ -15.0f, 0.0f, 0.0f };
            thirdPersonCamera->SetDistanceAndRotation(3.0f, eulerAngle);

            thirdPersonCamera->init();
            EntitySystem::UpdateEntity(entity);

            mThirdPersonCamera = thirdPersonCamera;
            SetActiveCamera(mThirdPersonCamera);
        }
        mPlayerMeshRenderer = entity->GetComponent<MeshRendererComponent>();
    }

    {// Map
        auto * entity = sponzaPrefab.Clone(GetRootEntity(), Prefab::CloneEntityOptions {.name = "Sponza"});
        if (auto const ptr = entity->GetComponent<TransformComponent>().lock())
        {
            float position[3]{ 0.4f, 2.0f, -6.0f };
            float eulerAngle[3]{ 180.0f, -90.0f, 0.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            ptr->UpdateTransform(position, eulerAngle, scale);
        }
    }
    
    {// Directional light
        auto * entity = EntitySystem::CreateEntity("Directional light", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
        MFA_ASSERT(colorComponent != nullptr);
        float const lightScale = 5.0f;
        float lightColor[3] {
            (252.0f/256.0f) * lightScale,
            (212.0f/256.0f) * lightScale,
            (64.0f/256.0f) * lightScale
        };
        colorComponent->SetColor(lightColor);

        auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdateRotation(glm::vec3(90.0f, 0.0f, 0.0f));
        
        entity->AddComponent<DirectionalLightComponent>();

        entity->SetActive(true);
        EntitySystem::InitEntity(entity);
    }

    //{// Fire
    //    auto * particlePipeline = SceneManager::GetPipeline<ParticlePipeline>();
    //    MFA_ASSERT(particlePipeline != nullptr);

    //    if (particlePipeline->essenceExists("Fire") == false){// Fire essence
    //        particlePipeline->addEssence(
    //            std::make_shared<FireEssence>(
    //                "Fire",
    //                RC::AcquireGpuTexture("images/fire/particle_fire.ktx"),
    //                FireEssence::Options {
    //                    .fireRadius = FireRadius,
    //                    .particleCount = 512
    //                }
    //            )
    //        );
    //    }

    //    {// Fire instances
    //        auto const createFireInstance = [this, particlePipeline](glm::vec3 const & position)->void
    //        {
    //            auto * entity = EntitySystem::CreateEntity("FireInstance", GetRootEntity());
    //            MFA_ASSERT(entity != nullptr);

    //            auto const transform = entity->AddComponent<TransformComponent>().lock();
    //            MFA_ASSERT(transform != nullptr);
    //            transform->UpdatePosition(position);
    //            
    //            entity->AddComponent<MeshRendererComponent>(
    //                *particlePipeline,
    //                "Fire"
    //            );
    //            entity->AddComponent<AxisAlignedBoundingBoxComponent>(
    //                glm::vec3{ 0.0f, -0.8f, 0.0f },
    //                glm::vec3{ FireRadius, 1.0f, FireRadius }
    //            );
    //            entity->AddComponent<ColorComponent>(glm::vec3{ 1.0f, 0.0f, 0.0f });

    //            auto const boundingVolumeRenderer = entity->AddComponent<BoundingVolumeRendererComponent>().lock();
    //            MFA_ASSERT(boundingVolumeRenderer != nullptr);
    //            boundingVolumeRenderer->SetActive(true);

    //            EntitySystem::InitEntity(entity);
    //        };
    //        createFireInstance(glm::vec3 {0.0f, 0.0f, 0.0f});
    //    }
    //}

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    mDebugRenderPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
    MFA_ASSERT(mDebugRenderPipeline != nullptr);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Update(float const deltaTimeInSec)
{
    if (auto const playerTransform = mPlayerTransform.lock())
    {// Soldier
        static constexpr float SoldierSpeed = 4.0f;
        auto const inputForwardMove = IM::GetForwardMove();
        auto const inputRightMove = IM::GetRightMove();
        if (inputForwardMove != 0.0f || inputRightMove != 0.0f)
        {
            float position[3]{};
            playerTransform->GetLocalPosition(position);
            float scale[3]{};
            playerTransform->GetScale(scale);
            float targetEulerAngles[3]{};
            playerTransform->GetRotation(targetEulerAngles);

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
            playerTransform->GetRotation(currentEulerAngles);

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
            Matrix::RotateWithEulerAngle(rotationMatrix, nextAngles);

            glm::vec4 forwardDirection(
                Math::ForwardVector4[0],
                Math::ForwardVector4[1],
                Math::ForwardVector4[2],
                Math::ForwardVector4[3]
            );
            forwardDirection = forwardDirection * rotationMatrix;
            forwardDirection = glm::normalize(forwardDirection);
            forwardDirection *= 1 * deltaTimeInSec * SoldierSpeed;

            position[0] += forwardDirection[0];
            position[1] += forwardDirection[1];
            position[2] += forwardDirection[2];

            playerTransform->UpdateTransform(
                position,
                nextAngles,
                scale
            );
            // TODO What should we do for animations ?
            if (auto const meshRendererPtr = mPlayerMeshRenderer.lock())
            {
                if (auto * variant = static_cast<PBR_Variant *>(meshRendererPtr->getVariant()))
                {
                    variant->SetActiveAnimation("SwordAndShieldRun", { .transitionDuration = 0.3f });
                }
            }
        }
        else
        {
            if (auto const meshRendererPtr = mPlayerMeshRenderer.lock())
            {
                if (auto * variant = static_cast<PBR_Variant *>(meshRendererPtr->getVariant()))
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
    
}

//-------------------------------------------------------------------------------------------------

bool Demo3rdPersonScene::RequiresUpdate()
{
    return true;
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::onUI() const
{
    UI::BeginWindow("3rd person scene");

    bool enableDebugPipeline = mDebugRenderPipeline->isActive();
    UI::Checkbox("Debug pipeline", &enableDebugPipeline);
    mDebugRenderPipeline->changeActivationStatus(enableDebugPipeline);

    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------
