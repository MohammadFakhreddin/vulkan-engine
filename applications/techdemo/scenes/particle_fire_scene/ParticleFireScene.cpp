#include "ParticleFireScene.hpp"

//-------------------------------------------------------------------------------------------------

ParticleFireScene::ParticleFireScene()
    : Scene()
{}

//-------------------------------------------------------------------------------------------------

ParticleFireScene::~ParticleFireScene()
{}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Init()
{
    Scene::Init();
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPreRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnPreRender(deltaTimeInSec, recordState);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnRender(deltaTimeInSec, recordState);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::OnPostRender(float const deltaTimeInSec, MFA::RT::CommandRecordState & recordState)
{
    Scene::OnPostRender(deltaTimeInSec, recordState);
}

//-------------------------------------------------------------------------------------------------

void ParticleFireScene::Shutdown()
{
    Scene::Shutdown();
}

//-------------------------------------------------------------------------------------------------
