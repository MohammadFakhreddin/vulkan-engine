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
    
        //struct Options {
        //    float particleMinLife = 1.0f;
        //    float particleMaxLife = 1.5f;
        //    float particleMinSpeed = 1.0f;
        //    float particleMaxSpeed = 2.0f;
        //    float fireRadius = 0.7f;
        //    float fireHorizontalMovement[2] {1.0f, 1.0f};
        //    float fireInitialPointSize = 500.0f;
        //    float fireTargetExtend[2] {1920.0f, 1080.0f};
        //    int particleCount = 512;//1024;
        //    float fireAlpha = 0.0001f;//0.001f;
        //};

        struct FireParams
        {
            float initialPointSize = 500.0f;
            float targetExtend [2] {1920.0f, 1080.0f};
        };
        
        explicit FireEssence(
            std::string const & name,
            uint32_t maxInstanceCount,
            std::vector<std::shared_ptr<RT::GpuTexture>> fireTextures,
            // TODO Smoke texture
            FireParams fireParams = {},
            AS::Particle::Params params = {}
        );
        
        //explicit FireEssence(
        //    std::string const & name,
        //    std::shared_ptr<RT::GpuTexture> const & textures,
        //    // TODO Smoke texture
        //    Params const & options
        //);

        ~FireEssence() override;

        FireEssence & operator= (FireEssence && rhs) noexcept = delete;
        FireEssence (FireEssence const &) noexcept = delete;
        FireEssence (FireEssence && rhs) noexcept = delete;
        FireEssence & operator = (FireEssence const &) noexcept = delete;
        
    private:

        void init();

        void computePointSize();

        void updateParamsIfChanged(AS::Particle::Params const & newParams);

        /*Params prepareConstructorParams(
            std::shared_ptr<RT::GpuTexture> const & fireTexture,
            std::string const & name,
            Options const & options
        );
        std::shared_ptr<AS::Particle::Mesh> createMesh(Options const & options);
        */
        //Options mOptions {};
    
        float mInitialPointSize = 0.0f;

        SignalId mResizeSignal = -1;

        FireParams mFireParams {};

        AS::Particle::Params mParams {};

        //Vertex * mVertices = nullptr;

    };
};
