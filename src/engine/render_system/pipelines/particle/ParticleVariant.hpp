#pragma once

#include "engine/render_system/pipelines/VariantBase.hpp"

namespace MFA
{
    class ParticleEssence;

    class ParticleVariant final : public VariantBase
    {
    public:

        explicit ParticleVariant(ParticleEssence const * essence);
        ~ParticleVariant() override;

        ParticleVariant(ParticleVariant const &) noexcept = delete;
        ParticleVariant(ParticleVariant &&) noexcept = delete;
        ParticleVariant & operator= (ParticleVariant const & rhs) noexcept = delete;
        ParticleVariant & operator= (ParticleVariant && rhs) noexcept = delete;

        bool getTransform(float outTransform[16]) const;

        void Draw(RT::CommandRecordState const & recordState) const;

    protected:

        void internalInit() override;

    private:
        uint32_t mIndicesCount = 0;
    };
}
