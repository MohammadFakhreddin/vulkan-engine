#pragma once

#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"

class ParticleFireScene final : public MFA::Scene
{
public:

    explicit ParticleFireScene();
    ~ParticleFireScene() override;

    ParticleFireScene (ParticleFireScene const &) noexcept = delete;
    ParticleFireScene (ParticleFireScene &&) noexcept = delete;
    ParticleFireScene & operator = (ParticleFireScene const &) noexcept = delete;
    ParticleFireScene & operator = (ParticleFireScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void OnPostRender(float deltaTimeInSec) override;

    void Shutdown() override;

private:

    void createFireEssence();

    void createFireInstance();

    void createCamera();

    MFA::ParticlePipeline mParticlePipeline {this};

    std::shared_ptr<MFA::RT::SamplerGroup> mSamplerGroup {};
    std::shared_ptr<MFA::RT::GpuTexture> mErrorTexture {};
    std::shared_ptr<MFA::AssetSystem::Model> mFireModel {};

};
