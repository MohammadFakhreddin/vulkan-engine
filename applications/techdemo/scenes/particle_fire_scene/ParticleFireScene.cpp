#include "ParticleFireScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "engine/camera/ObserverCameraComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"
#include "engine/entity_system/components/AxisAlignedBoundingBoxComponent.hpp"
#include "engine/entity_system/components/BoundingVolumeRendererComponent.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/MeshRendererComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/particle/FireEssence.hpp"

using namespace MFA;

static constexpr float FireRadius = 0.7f;

//-------------------------------------------------------------------------------------------------

ParticleFireScene::ParticleFireScene()
    : Scene()
{}

//-------------------------------------------------------------------------------------------------

ParticleFireScene::~ParticleFireScene() = default;

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Init()
{
    Scene::Init();

    mParticlePipeline = SceneManager::GetPipeline<ParticlePipeline>();
    MFA_ASSERT(mParticlePipeline != nullptr);

    mDebugRendererPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
    MFA_ASSERT(mDebugRendererPipeline != nullptr);

    createFireEssence();

    createFireInstance(glm::vec3 {0.0f, 0.0f, 0.0f});

    createCamera();
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Update(float const deltaTimeInSec)
{
    Scene::Update(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Shutdown()
{
    Scene::Shutdown();
}

//-------------------------------------------------------------------------------------------------

bool ParticleFireScene::RequiresUpdate()
{
    return true;
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireEssence() const
{
    if (mParticlePipeline->hasEssence("ParticleSceneFire") == false)
    {
        mParticlePipeline->addEssence(
            std::make_shared<FireEssence>(
                "ParticleSceneFire",
                100,
                std::vector {RC::AcquireGpuTexture("images/fire/particle_fire.ktx")},
                FireEssence::FireParams {},
                AS::Particle::Params {
                    .count = 1024,
                    .radius = FireRadius
                }
            )
        );
    }
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireInstance(glm::vec3 const & position) const
{
    auto * entity = EntitySystem::CreateEntity("FireInstance", GetRootEntity());
    MFA_ASSERT(entity != nullptr);

    auto const transform = entity->AddComponent<TransformComponent>().lock();
    MFA_ASSERT(transform != nullptr);
    transform->UpdatePosition(position);
    
    entity->AddComponent<MeshRendererComponent>(
        *mParticlePipeline,
        "ParticleSceneFire"
    );
    entity->AddComponent<AxisAlignedBoundingBoxComponent>(
        glm::vec3{ 0.0f, -0.8f, 0.0f },
        glm::vec3{ FireRadius, 1.0f, FireRadius }
    );
    entity->AddComponent<ColorComponent>(glm::vec3{ 1.0f, 0.0f, 0.0f });

    auto const boundingVolumeRenderer = entity->AddComponent<BoundingVolumeRendererComponent>(*mDebugRendererPipeline).lock();
    MFA_ASSERT(boundingVolumeRenderer != nullptr);
    boundingVolumeRenderer->SetActive(true);

    EntitySystem::InitEntity(entity);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createCamera()
{
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 3000.0f;
    static constexpr float FOV = 80;

    auto * entity = EntitySystem::CreateEntity("CameraEntity", GetRootEntity());
    MFA_ASSERT(entity != nullptr);

    auto const observerCamera = entity->AddComponent<ObserverCameraComponent>(FOV, Z_NEAR, Z_FAR).lock();
    MFA_ASSERT(observerCamera != nullptr);
    SetActiveCamera(observerCamera);

    EntitySystem::InitEntity(entity);
}

//-------------------------------------------------------------------------------------------------
