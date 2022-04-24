#pragma once

#include "ParticleEssence.hpp"
#include "engine/BedrockSignalTypes.hpp"
#include "engine/render_system/RenderTypesFWD.hpp"

#include <string>

namespace MFA
{
    struct FireParams
    {
        float initialPointSize = 500.0f;
        float targetExtend [2] {1920.0f, 1080.0f};
    };
        
    class FireEssence final : public ParticleEssence
    {
    public:
        
        explicit FireEssence(
            std::string const & name,
            uint32_t maxInstanceCount,
            std::vector<std::shared_ptr<RT::GpuTexture>> fireTextures,
            // TODO Smoke texture
            FireParams fireParams = FireParams {},
            AS::Particle::Params params = AS::Particle::Params {}
        );

        ~FireEssence() override;

        FireEssence & operator= (FireEssence && rhs) noexcept = delete;
        FireEssence (FireEssence const &) noexcept = delete;
        FireEssence (FireEssence && rhs) noexcept = delete;
        FireEssence & operator = (FireEssence const &) noexcept = delete;
        
    private:

        void init();

        void computePointSize();

        float mInitialPointSize = 0.0f;

        SignalId mResizeSignal = -1;

        FireParams mFireParams {};
    };
};
