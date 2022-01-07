#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"

namespace MFA
{
    class DebugEssence final : public EssenceBase
    {
    public:

        explicit DebugEssence(std::shared_ptr<RT::GpuModel> const & gpuModel, uint32_t indexCount);
        ~DebugEssence() override;
        
        DebugEssence & operator= (DebugEssence && rhs) noexcept = delete;
        DebugEssence (DebugEssence const &) noexcept = delete;
        DebugEssence (DebugEssence && rhs) noexcept = delete;
        DebugEssence & operator = (DebugEssence const &) noexcept = delete;

        [[nodiscard]]
        uint32_t getIndicesCount() const;

    private:

        uint32_t const mIndicesCount;

    };
}
