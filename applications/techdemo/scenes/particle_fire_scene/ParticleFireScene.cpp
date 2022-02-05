#include "ParticleFireScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/asset_system/AssetParticleMesh.hpp"
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

using namespace MFA;

static constexpr float ParticleMinLife = 1.0f;
static constexpr float ParticleMaxLife = 1.5f;
static constexpr float ParticleMinSpeed = 1.0f;
static constexpr float ParticleMaxSpeed = 2.0f;
static constexpr float FireRadius = 0.7f;
static constexpr float FireHorizontalMovement[2] {1.0f, 1.0f};
static constexpr float FireInitialPointSize = 500.0f;
static constexpr float FireTargetExtend[2] {1920.0f, 1080.0f}; 
static constexpr int ParticleCount = 1024;
static constexpr float FireAlpha = 0.001f;

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

    computeFirePointSize();

    createFireEssence();

    createFireInstance(glm::vec3 {0.0f, 0.0f, 0.0f});

    createCamera();

    mResizeSignalId = RF::AddResizeEventListener([this]()->void {onResize();});
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Update(float const deltaTimeInSec)
{
    Scene::Update(deltaTimeInSec);

    // TODO Try to use job system
    for (int i = 0; i < mFireVerticesCount; ++i)
    {
        auto & vertex = mFireVertices[i];
        // UpVector is reverse
        glm::vec3 const deltaPosition = -deltaTimeInSec * vertex.speed * Math::UpVector3;
        vertex.localPosition += deltaPosition;
        vertex.localPosition += Math::Random(-FireHorizontalMovement[0], FireHorizontalMovement[0])
            * deltaTimeInSec * Math::RightVector3;
        vertex.localPosition += Math::Random(-FireHorizontalMovement[1], FireHorizontalMovement[1])
            * deltaTimeInSec * Math::ForwardVector3;
        
        vertex.remainingLifeInSec -= deltaTimeInSec;
        if (vertex.remainingLifeInSec <= 0)
        {
            vertex.localPosition = vertex.initialLocalPosition;

            vertex.speed = Math::Random(ParticleMinSpeed, ParticleMaxSpeed);
            vertex.remainingLifeInSec = Math::Random(ParticleMinLife, ParticleMaxLife);;
            vertex.totalLifeInSec = vertex.remainingLifeInSec;
        }

        auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;
        
        vertex.alpha = FireAlpha;

        vertex.pointSize = mFirePointSize * lifePercentage;
    }
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Shutdown()
{
    Scene::Shutdown();
    RF::RemoveResizeEventListener(mResizeSignalId);
}

//-------------------------------------------------------------------------------------------------

bool ParticleFireScene::isDisplayPassDepthImageInitialLayoutUndefined()
{
    return true;
}

//-------------------------------------------------------------------------------------------------

bool ParticleFireScene::RequiresUpdate()
{
    return true;
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::onResize()
{
    computeFirePointSize();
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireEssence()
{
    auto const fireModel = std::make_shared<AS::Model>(
        createFireMesh(),
        createFireTexture()
    );

    mParticlePipeline->createEssenceWithModel(fireModel, "Fire");
}

//-------------------------------------------------------------------------------------------------

std::shared_ptr<MFA::AssetSystem::Particle::Mesh> ParticleFireScene::createFireMesh()
{
    auto fireMesh = std::make_shared<AS::Particle::Mesh>(100);
    auto const verticesCount = ParticleCount;
    auto const indicesCount = ParticleCount;
    auto const vertexBuffer = Memory::Alloc(verticesCount * sizeof(AS::Particle::Vertex));
    auto const indexBuffer = Memory::Alloc(indicesCount * sizeof(AS::Index));
    fireMesh->initForWrite(
        verticesCount,
        indicesCount,
        vertexBuffer,
        indexBuffer
    );

    auto * vertexItems = vertexBuffer->memory.as<AS::Particle::Vertex>();
    auto * indexItems = indexBuffer->memory.as<AS::Index>();

    for (int i = 0; i < verticesCount; ++i)
    {
        auto & vertex = vertexItems[i];

        auto const yaw = Math::Random(-Math::PiFloat, Math::PiFloat);
        auto const distanceFromCenter = Math::Random(0.0f, FireRadius) * Math::Random(0.5f, 1.0f);
        
        auto transform = glm::identity<glm::mat4>();
        Matrix::RotateWithRadians(transform, glm::vec3{0.0f, yaw, 0.0f});

        glm::vec3 const position = transform * glm::vec4 {distanceFromCenter, 0.0f, 0.0f, 1.0f};

        vertex.localPosition = position;
        vertex.initialLocalPosition = vertex.localPosition;

        vertex.textureIndex = 0;

        vertex.color[0] = 1.0f;
        vertex.color[1] = 0.0f;
        vertex.color[2] = 0.0f;

        vertex.speed = Math::Random(ParticleMinSpeed, ParticleMaxSpeed);
        vertex.remainingLifeInSec = Math::Random(ParticleMinLife, ParticleMaxLife);
        vertex.totalLifeInSec = vertex.remainingLifeInSec;

        auto const lifePercentage = vertex.remainingLifeInSec / vertex.totalLifeInSec;
        
        vertex.alpha = FireAlpha;

        vertex.pointSize = mFirePointSize * lifePercentage;

        indexItems[i] = i;
    } 

    mFireVerticesCount = verticesCount;
    mFireVertices = vertexItems;
    MFA_ASSERT(mFireVertices != nullptr);

    return fireMesh;
}

//-------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<AS::Texture>> ParticleFireScene::createFireTexture()
{
    std::vector<std::shared_ptr<AS::Texture>> textures {};
    textures.emplace_back(RC::AcquireCpuTexture("images\\fire\\particle_fire.ktx"));
    MFA_ASSERT(textures.back() != nullptr);
    return textures;
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireInstance(glm::vec3 const & position)
{
    auto * entity = EntitySystem::CreateEntity("FireInstance", GetRootEntity());
    MFA_ASSERT(entity != nullptr);

    auto const transform = entity->AddComponent<TransformComponent>().lock();
    MFA_ASSERT(transform != nullptr);
    transform->UpdatePosition(position);
    
    entity->AddComponent<MeshRendererComponent>(
        *mParticlePipeline,
        "Fire"
    );
    entity->AddComponent<AxisAlignedBoundingBoxComponent>(
        glm::vec3{ 0.0f, -0.8f, 0.0f },
        glm::vec3{ FireRadius, 1.0f, FireRadius }
    );
    entity->AddComponent<ColorComponent>(glm::vec3{ 1.0f, 0.0f, 0.0f });

    auto const boundingVolumeRenderer = entity->AddComponent<BoundingVolumeRendererComponent>(*mDebugRendererPipeline).lock();
    MFA_ASSERT(boundingVolumeRenderer != nullptr);
    boundingVolumeRenderer->SetActive(false);

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

    /* entity->AddComponent<DirectionalLightComponent>();

     auto const colorComponent = entity->AddComponent<ColorComponent>().lock();
     MFA_ASSERT(colorComponent != nullptr);
     float const lightScale = 5.0f;
     float lightColor[3]{
         1.0f * lightScale,
         1.0f * lightScale,
         1.0f * lightScale
     };
     colorComponent->SetColor(lightColor);

     auto const transformComponent = entity->AddComponent<TransformComponent>().lock();
     MFA_ASSERT(transformComponent != nullptr);
     transformComponent->UpdateRotation(glm::vec3(90.0f, 0.0f, 0.0f));*/

    EntitySystem::InitEntity(entity);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::computeFirePointSize()
{
    auto const surfaceCapabilities = RF::GetSurfaceCapabilities();
    mFirePointSize = FireInitialPointSize * (static_cast<float>(surfaceCapabilities.currentExtent.width) / FireTargetExtend[0]);
}

//-------------------------------------------------------------------------------------------------
