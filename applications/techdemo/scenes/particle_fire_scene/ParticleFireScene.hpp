#pragma once
#include "engine/scene_manager/Scene.hpp"

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

    void OnPostRender(float deltaTimeInSec, MFA::RT::CommandRecordState & recordState) override;

    void Shutdown() override;

private:

};
