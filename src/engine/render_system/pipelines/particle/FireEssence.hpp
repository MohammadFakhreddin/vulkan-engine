#pragma once

#include "ParticleEssence.hpp"
#include "engine/BedrockSignalTypes.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"

#include <string>

namespace MFA
{
    class FireEssence final : public ParticleEssence
    {
    public:
    
        struct Options {
            float particleMinLife = 1.0f;
            float particleMaxLife = 1.5f;
            float particleMinSpeed = 1.0f;
            float particleMaxSpeed = 2.0f;
            float fireRadius = 0.7f;
            float fireHorizontalMovement[2] {1.0f, 1.0f};
            float fireInitialPointSize = 500.0f;
            float fireTargetExtend[2] {1920.0f, 1080.0f};
            int particleCount = 512;//1024;
            float fireAlpha = 0.0001f;//0.001f;
        };
        
        explicit FireEssence(
            std::string const & name,
            std::shared_ptr<RT::GpuTexture> const & fireTexture
            // TODO Smoke texture
        );
        
        explicit FireEssence(
            std::string const & name,
            std::shared_ptr<RT::GpuTexture> const & fireTexture,
            // TODO Smoke texture
            Options options
        );

        ~FireEssence() override;
        
        void update(
            RT::CommandRecordState const & recordState,
            float deltaTimeInSec,
            VariantsList const & variants
        ) override;
        
    private:
        
        void computeFirePointSize();

        std::shared_ptr<AS::Model> createModel(std::shared_ptr<RT::GpuTexture> const & fireTexture);
        std::shared_ptr<AS::Particle::Mesh> createMesh();
        
        Options mOptions {};
    
        float mInitialPointSize = 0.0f;

        SignalId mResizeSignal = -1;

        MFA::AssetSystem::Particle::Vertex * mVertices = nullptr;

    };
};
