#pragma once

#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"

#include <vec3.hpp>

namespace MFA
{
    namespace AssetSystem
    {
        namespace Particle
        {
            class Mesh;
            struct Vertex;
        }
    }
}

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

    bool isDisplayPassDepthImageInitialLayoutUndefined() override;

    void OnResize() override;

private:

    void createFireEssence();
    std::shared_ptr<MFA::AssetSystem::Particle::Mesh> createFireMesh();
    std::vector<std::shared_ptr<MFA::AssetSystem::Texture>> createFireTexture(); 
    void createFireInstance(glm::vec3 const & position);

    void createCamera();

    void computeFirePointSize();

    MFA::ParticlePipeline mParticlePipeline {this};
    MFA::DebugRendererPipeline mDebugPipeline {};

    std::shared_ptr<MFA::RT::GpuTexture> mErrorTexture {};
    
    int mFireVerticesCount = 0;
    MFA::AssetSystem::Particle::Vertex * mFireVertices = nullptr;

    float firePointSize = 0.0f;

};
