#include "DebugEssence.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    DebugEssence::DebugEssence(std::shared_ptr<RT::GpuModel> const & gpuModel, uint32_t indexCount)
        : EssenceBase(gpuModel)
        , mIndicesCount(indexCount)
    {
        MFA_ASSERT(indexCount > 0);
    }

    //-------------------------------------------------------------------------------------------------

    DebugEssence::~DebugEssence() = default;

    //-------------------------------------------------------------------------------------------------

    uint32_t DebugEssence::getIndicesCount() const
    {
        return mIndicesCount;
    }

    //-------------------------------------------------------------------------------------------------

}
