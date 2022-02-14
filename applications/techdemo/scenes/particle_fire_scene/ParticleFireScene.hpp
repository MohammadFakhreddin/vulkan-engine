#pragma once

#include "engine/scene_manager/Scene.hpp"
#include "engine/BedrockSignalTypes.hpp"

#include <vec3.hpp>

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

    //void onResize();

    void createFireEssence();
    std::shared_ptr<MFA::AssetSystem::Particle::Mesh> createFireMesh();
    std::vector<std::shared_ptr<MFA::AssetSystem::Texture>> createFireTexture(); 
    void createFireInstance(glm::vec3 const & position);

    void createCamera();

    //void computeFirePointSize();
    
    // int mFireVerticesCount = 0;
    // MFA::AssetSystem::Particle::Vertex * mFireVertices = nullptr;

    // float mFirePointSize = 0.0f;

    MFA::ParticlePipeline * mParticlePipeline = nullptr;
    MFA::DebugRendererPipeline * mDebugRendererPipeline = nullptr;

    // int mResizeSignalId = MFA::InvalidSignalId;

};
