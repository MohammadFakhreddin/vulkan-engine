#include "ParticleFireScene.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "tools/Importer.hpp"

using namespace MFA;

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
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPreRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnPreRender(deltaTimeInSec, recordState);

    mParticlePipeline.PreRender(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnRender(deltaTimeInSec, recordState);

    mParticlePipeline.Render(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPostRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnPostRender(deltaTimeInSec, recordState);

    mParticlePipeline.PostRender(recordState, deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Shutdown()
{
    Scene::Shutdown();

    mParticlePipeline.Shutdown();
}

//-------------------------------------------------------------------------------------------------
