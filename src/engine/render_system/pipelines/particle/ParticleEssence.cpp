#include "ParticleEssence.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    ParticleEssence::ParticleEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        uint32_t const indicesCount
    )
        : EssenceBase(gpuModel)
        , mIndicesCount(indicesCount)
    {
        MFA_ASSERT(indicesCount > 0);
    }

    //-------------------------------------------------------------------------------------------------

    ParticleEssence::~ParticleEssence() = default;

    //-------------------------------------------------------------------------------------------------

    uint32_t ParticleEssence::getIndicesCount() const
    {
        return mIndicesCount;
    }

    //-------------------------------------------------------------------------------------------------

}
