#include "ParticleFireScene.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
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

using namespace MFA;

static constexpr float ParticleLife = 2.0f;
static constexpr float ParticleSpeed = 1.0f;

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

    // Sampler
    mSamplerGroup = RF::CreateSampler(RT::CreateSamplerParams{});
    // Error texture
    // TODO RC must support importing texture and mesh as well
    auto const cpuErrorTexture = Importer::CreateErrorTexture();
    mErrorTexture = RF::CreateTexture(*cpuErrorTexture);

    mParticlePipeline.Init(mSamplerGroup, mErrorTexture);
    mDebugPipeline.Init();
    RegisterPipeline(&mParticlePipeline);
    RegisterPipeline(&mDebugPipeline);

    createFireEssence();

    createFireInstance();

    createCamera();

}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPreRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnPreRender(deltaTimeInSec, recordState);

    mParticlePipeline.PreRender(recordState, deltaTimeInSec);
    mDebugPipeline.PreRender(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnRender(deltaTimeInSec, recordState);

    mParticlePipeline.Render(recordState, deltaTimeInSec);
    mDebugPipeline.Render(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPostRender(float const deltaTimeInSec)
{
    Scene::OnPostRender(deltaTimeInSec);

    //auto * vertexItems = mFireModel->mesh->getVertexBuffer()->memory.as<AS::Particle::Vertex>();
    for (int i = 0; i < mFireVerticesCount; ++i)
    {
        auto & vertex = mFireVertices[i];


        // Is UpVector reverse ?
        auto const deltaPosition = -deltaTimeInSec * ParticleSpeed * Math::UpVector;
        vertex.localPosition[0] += deltaPosition.x;
        vertex.localPosition[1] += deltaPosition.y;
        vertex.localPosition[2] += deltaPosition.z;

        vertex.remainingLifeInSec -= deltaTimeInSec;
        if (vertex.remainingLifeInSec <= 0)
        {
            Copy<3>(vertex.localPosition, vertex.initialLocalPosition);
            vertex.remainingLifeInSec = ParticleLife;
        }

        vertex.alpha = vertex.remainingLifeInSec / ParticleLife;
    }
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Shutdown()
{
    Scene::Shutdown();

    mParticlePipeline.Shutdown();
    mDebugPipeline.Shutdown();
}

//-------------------------------------------------------------------------------------------------

bool ParticleFireScene::isDisplayPassDepthImageInitialLayoutUndefined()
{
    return true;
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireEssence()
{
    auto fireMesh = std::make_shared<AS::Particle::Mesh>(100);
    auto const verticesCount = 10000;
    auto const indicesCount = 10000;
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

    static constexpr float radius = 1.0f;

    for (int i = 0; i < verticesCount; ++i)
    {
        auto & vertex = vertexItems[i];

        vertex.localPosition[0] = Math::Random(-radius, radius);
        vertex.localPosition[1] = Math::Random(-radius, radius);
        vertex.localPosition[2] = Math::Random(-radius, radius);

        Copy<3>(vertex.initialLocalPosition, vertex.localPosition);

        vertex.textureIndex = -1;

        vertex.uv[0] = 0.0f;
        vertex.uv[1] = 0.0f;

        vertex.color[0] = 1.0f;
        vertex.color[1] = 0.0f;
        vertex.color[2] = 0.0f;
        
        vertex.remainingLifeInSec = Math::Random(0.01f, ParticleLife);

        vertex.alpha = vertex.remainingLifeInSec / ParticleLife;

        indexItems[i] = i;
    }

    auto const fireModel = std::make_shared<AS::Model>(
        fireMesh,
        std::vector<std::shared_ptr<AS::Texture>>{}
    );

    mParticlePipeline.CreateEssenceWithModel(fireModel, "Fire");

    mFireVerticesCount = verticesCount;
    mFireVertices = vertexItems;
    MFA_ASSERT(mFireVertices != nullptr);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::createFireInstance()
{
    auto * entity = EntitySystem::CreateEntity("FireInstance", GetRootEntity());
    MFA_ASSERT(entity != nullptr);

    entity->AddComponent<TransformComponent>();
    entity->AddComponent<MeshRendererComponent>(mParticlePipeline, "Fire");
    entity->AddComponent<AxisAlignedBoundingBoxComponent>(
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 1.0f }
    );
    entity->AddComponent<ColorComponent>(glm::vec3{ 1.0f, 0.0f, 0.0f });
    entity->AddComponent<BoundingVolumeRendererComponent>(mDebugPipeline);

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
