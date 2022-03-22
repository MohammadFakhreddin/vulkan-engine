#pragma once

#include "engine/render_system/pipelines/EssenceBase.hpp"

namespace MFA
{
    class DebugEssence final : public EssenceBase
    {
    public:

        explicit DebugEssence(std::shared_ptr<RT::GpuModel> gpuModel, uint32_t indexCount);
        ~DebugEssence() override;
        
        DebugEssence & operator= (DebugEssence && rhs) noexcept = delete;
        DebugEssence (DebugEssence const &) noexcept = delete;
        DebugEssence (DebugEssence && rhs) noexcept = delete;
        DebugEssence & operator = (DebugEssence const &) noexcept = delete;

        [[nodiscard]]
        uint32_t getIndicesCount() const;

        void setGraphicDescriptorSet(RT::DescriptorSetGroup const & descriptorSet);

        void bindForGraphicPipeline(RT::CommandRecordState const & recordState) const;

    private:

        void bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const;

        void bindVertexBuffer(RT::CommandRecordState const & recordState) const;

        void bindIndexBuffer(RT::CommandRecordState const & recordState) const;

        std::shared_ptr<RT::GpuModel> mGpuModel;
        uint32_t const mIndicesCount;

        RT::DescriptorSetGroup mGraphicDescriptorSet {};

    };
}
