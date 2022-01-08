#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"

namespace MFA
{
    class ParticleEssence final : public EssenceBase
    {
    public:

        explicit ParticleEssence(std::shared_ptr<RT::GpuModel> const & gpuModel, uint32_t indicesCount);
        ~ParticleEssence() override;
        
        ParticleEssence & operator= (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence (ParticleEssence const &) noexcept = delete;
        ParticleEssence (ParticleEssence && rhs) noexcept = delete;
        ParticleEssence & operator = (ParticleEssence const &) noexcept = delete;

        [[nodiscard]]
        uint32_t getIndicesCount() const;

    private:

        uint32_t const mIndicesCount;
    };
}
