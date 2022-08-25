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
#include "engine/entity_system/components/CapsuleColliderComponent.hpp"
#include "engine/entity_system/components/MeshColliderComponent.hpp"
#include "engine/entity_system/components/RigidbodyComponent.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Variant.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "tools/PrefabFileStorage.hpp"
#include "engine/render_system/pipelines/particle/FireEssence.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "tools/Prefab.hpp"

namespace MFA
{
    class BoundingVolumeRendererComponent;
}

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

    Prefab soldierPrefab{ EntitySystem::CreateEntity("SolderPrefab", nullptr) };
    Prefab sponzaPrefab{ EntitySystem::CreateEntity("SponzaPrefab", nullptr) };

    PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams{
        .fileAddress = Path::ForReadWrite("prefabs/soldier.json"),
        .prefab = &soldierPrefab
    });

    PrefabFileStorage::Deserialize(PrefabFileStorage::DeserializeParams{
        .fileAddress = Path::ForReadWrite("prefabs/sponza3.json"),
        .prefab = &sponzaPrefab
    });

    std::shared_ptr<TransformComponent> playerTransform = nullptr;
    {// Playable soldier
        auto * entity = soldierPrefab.Clone(GetRootEntity(), Prefab::CloneEntityOptions{ .name = "Playable soldier" });
        {// Transform
            float position[3]{ 0.0f, 2.0f, -5.0f };
            float eulerAngles[3]{ 0.0f, 180.0f, -180.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            playerTransform = entity->GetComponent<TransformComponent>();
            playerTransform->UpdateLocalTransform(
                position,
                eulerAngles,
                scale
            );
            mPlayerTransform = playerTransform;
        }
        mPlayerMeshRenderer = entity->GetComponent<MeshRendererComponent>();
        // TODO: We need to be able to draw colliders on editor as well
        // Capsule collider
        auto const capsuleCollider = entity->AddComponent<CapsuleCollider>(0.5f, 0.2f);
        // Rigidbody
        auto const rigidbody = entity->AddComponent<Rigidbody>(false, true, 1.0f);
        capsuleCollider->Init();
        rigidbody->LateInit();
        EntitySystem::UpdateEntity(entity);
        mPlayerRigidbody = rigidbody;
    }
    {// Camera
        auto const entity = EntitySystem::CreateEntity("Camera", GetRootEntity());

        auto const transform = entity->AddComponent<TransformComponent>();

        transform->UpdateLocalRotation(glm::vec3{ -15.0f, 0.0f, 0.0f });

        auto const camera = entity->AddComponent<ThirdPersonCameraComponent>(
            playerTransform,
            FOV,
            Z_NEAR,
            Z_FAR
        );

        camera->SetDistance(3.0f);

        EntitySystem::InitEntity(entity);

        mThirdPersonCamera = camera;
        SetActiveCamera(mThirdPersonCamera);
    }

    //for (float i = 0; i < 10.0f; i += 1.0f) {// Dummy soldiers
    //    for (float j = 0; j < 10.0f; j += 1.0f)
    //    {
    //        auto * entity = soldierPrefab.Clone(GetRootEntity(), Prefab::CloneEntityOptions {.name = "Soldier"});
    //        auto const transform = entity->GetComponent<TransformComponent>().lock();
    //        MFA_ASSERT(transform != nullptr);
    //        float position[3]{ 0.0f + i, 2.0f, -5.0f + j };
    //        transform->UpdatePosition(position);
    //    }
    //}

    Entity * sponzaEntity = nullptr;
    {// Map
        sponzaEntity = sponzaPrefab.Clone(GetRootEntity(), Prefab::CloneEntityOptions{ .name = "Sponza" });
        if (auto const ptr = sponzaEntity->GetComponent<TransformComponent>())
        {
            float position[3]{ 0.4f, 2.0f, -6.0f };
            float eulerAngle[3]{ 180.0f, -90.0f, 0.0f };
            float scale[3]{ 1.0f, 1.0f, 1.0f };
            ptr->UpdateLocalTransform(position, eulerAngle, scale);
        }
        {// Mesh collider
            auto const meshCollider = sponzaEntity->AddComponent<MeshColliderComponent>(true);
            meshCollider->Init();
            EntitySystem::UpdateEntity(sponzaEntity);
        }

        sponzaEntity->SetActive(true);
    }

    {// Directional light
        auto * entity = EntitySystem::CreateEntity("Directional light", GetRootEntity());
        MFA_ASSERT(entity != nullptr);

        auto const colorComponent = entity->AddComponent<ColorComponent>();
        MFA_ASSERT(colorComponent != nullptr);
        float const lightScale = 0.5f;
        float lightColor[3]{
            (252.0f / 256.0f) * lightScale,
            (212.0f / 256.0f) * lightScale,
            (64.0f / 256.0f) * lightScale
        };
        colorComponent->SetColor(lightColor);

        auto const transformComponent = entity->AddComponent<TransformComponent>();
        MFA_ASSERT(transformComponent != nullptr);
        transformComponent->UpdateLocalRotation(glm::vec3(90.0f, 0.0f, 0.0f));

        entity->AddComponent<DirectionalLightComponent>();

        entity->SetActive(true);
        EntitySystem::InitEntity(entity);
    }

    {// Fire
        RC::AcquireGpuTexture("images/fire/particle_fire.ktx", [this, sponzaEntity](std::shared_ptr<RT::GpuTexture> const & gpuTexture)->void
{
    auto * particlePipeline = SceneManager::GetPipeline<ParticlePipeline>();
    MFA_ASSERT(particlePipeline != nullptr);

    createFireEssence(gpuTexture);
    // TODO: Use getComponentInchildren
    // Fire instances
    createFireInstance(glm::vec3{ 3.9f, 1.0f, 1.45f }, sponzaEntity);
    createFireInstance(glm::vec3{ -4.95f, 1.0f, -1.45f }, sponzaEntity);
    createFireInstance(glm::vec3{ -4.95f, 1.0f, 1.45f }, sponzaEntity);
    createFireInstance(glm::vec3{ 3.9f, 1.0f, -1.45f }, sponzaEntity);
        });
    }

    mUIRecordId = UI::Register([this]()->void { onUI(); });

    mDebugRenderPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
    MFA_ASSERT(mDebugRenderPipeline != nullptr);
    mDebugRenderPipeline->changeActivationStatus(false);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Update(float const deltaTimeInSec)
{
    auto playerTransform = mPlayerTransform.lock();
    if (playerTransform == nullptr)
    {
        return;
    }
    auto playerRigidbody = mPlayerRigidbody.lock();
    if (playerRigidbody == nullptr)
    {
        return;
    }

    auto camera = mThirdPersonCamera.lock();
    if (camera == nullptr)
    {
        return;
    }

    auto playerMeshRenderer = mPlayerMeshRenderer.lock();
    if (playerMeshRenderer == nullptr)
    {
        return;
    }

    auto playerVariant = static_pointer_cast<PBR_Variant>(playerMeshRenderer->getVariant().lock());
    if (playerVariant == nullptr)
    {
        return;
    }

    static constexpr float SoldierSpeed = 4.0f;
    auto const inputForwardMove = IM::GetForwardMove();
    auto const inputRightMove = IM::GetRightMove();

    auto const forward = playerTransform->Forward();
    auto const right = playerTransform->Right();

    glm::vec3 velocity{};
    velocity += forward * inputForwardMove * SoldierSpeed;
    velocity += right * inputRightMove * SoldierSpeed;

    playerRigidbody->SetLinearVelocity(velocity);

    bool const isIdle = Matrix::IsNearZero(playerRigidbody->GetLinearVelocity());
    if (isIdle == false)
    {
        playerVariant->SetActiveAnimation("SwordAndShieldRun", { .transitionDuration = 0.3f });
    }
    else
    {
        //mSoldierVariant->SetActiveAnimation("SwordAndShieldIdle");
        playerVariant->SetActiveAnimation("Idle", { .transitionDuration = 0.3f });
    }
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::Shutdown()
{
    Scene::Shutdown();

    UI::UnRegister(mUIRecordId);

    mDebugRenderPipeline->changeActivationStatus(true);

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

void Demo3rdPersonScene::createFireEssence(std::shared_ptr<RT::GpuTexture> const & gpuTexture) const
{
    auto * particlePipeline = SceneManager::GetPipeline<ParticlePipeline>();
    MFA_ASSERT(particlePipeline != nullptr);

    if (particlePipeline->hasEssence("SponzaFire") == true)
    {// Fire essence
        return;
    }

    auto const fireEssence = std::make_shared<FireEssence>(
        "SponzaFire",
        100,   // TODO: Find a better number
        std::vector{ gpuTexture },
        MFA::FireParams{
            .initialPointSize = 300.0f
        },
        AS::Particle::Params{
            .count = 256,
            .minLife = 0.2f,
            .maxLife = 1.0f,
            .minSpeed = 0.5f,
            .maxSpeed = 1.0f,
            .radius = 0.1f,
        }
    );

    auto const addResult = particlePipeline->addEssence(fireEssence);
    MFA_ASSERT(addResult);
}

//-------------------------------------------------------------------------------------------------

void Demo3rdPersonScene::createFireInstance(glm::vec3 const & position, Entity * parent) const
{
    auto * particlePipeline = SceneManager::GetPipeline<ParticlePipeline>();
    MFA_ASSERT(particlePipeline != nullptr);

    auto * entity = EntitySystem::CreateEntity("FireInstance", parent);
    MFA_ASSERT(entity != nullptr);

    auto const transform = entity->AddComponent<TransformComponent>();
    MFA_ASSERT(transform != nullptr);
    transform->UpdateLocalPosition(position);

    entity->AddComponent<MeshRendererComponent>(
        particlePipeline,
        "SponzaFire"
    );
    entity->AddComponent<AxisAlignedBoundingBoxComponent>(
        glm::vec3{ 0.0f, -0.3f, 0.0f },
        glm::vec3{ 0.4f, 0.8f, 0.4f },
        true
    );
    entity->AddComponent<ColorComponent>(glm::vec3{ 1.0f, 0.0f, 0.0f });

    auto const boundingVolumeRenderer = entity->AddComponent<BoundingVolumeRendererComponent>();
    MFA_ASSERT(boundingVolumeRenderer != nullptr);
    boundingVolumeRenderer->SetActive(true);

    EntitySystem::InitEntity(entity);
}

//-------------------------------------------------------------------------------------------------
