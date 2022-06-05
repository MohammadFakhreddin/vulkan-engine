#pragma once

#include "engine/scene_manager/Scene.hpp"

#include <glm/vec3.hpp>

namespace MFA
{
    class ParticlePipeline;
    class DebugRendererPipeline;
}

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

    void Update(float deltaTimeInSec) override;

    void Shutdown() override;
    
    bool RequiresUpdate() override;
    
private:

    void createFireEssence() const;
    void createFireInstance(glm::vec3 const & position) const;

    void createCamera();
    
    MFA::ParticlePipeline * mParticlePipeline = nullptr;
    MFA::DebugRendererPipeline * mDebugRendererPipeline = nullptr;
    
};
