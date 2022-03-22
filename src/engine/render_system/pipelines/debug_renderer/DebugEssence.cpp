#include "DebugEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    DebugEssence::DebugEssence(std::shared_ptr<RT::GpuModel> gpuModel, uint32_t const indexCount)
        : EssenceBase(gpuModel->nameId)
        , mGpuModel(std::move(gpuModel))
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

    void DebugEssence::setGraphicDescriptorSet(RT::DescriptorSetGroup const & descriptorSet)
    {
        MFA_ASSERT(descriptorSet.IsValid());
        mGraphicDescriptorSet = descriptorSet;
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindForGraphicPipeline(RT::CommandRecordState const & recordState) const
    {
        bindGraphicDescriptorSet(recordState);
        bindVertexBuffer(recordState);
        bindIndexBuffer(recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindGraphicDescriptorSet(RT::CommandRecordState const & recordState) const
    {
        RF::AutoBindDescriptorSet(
            recordState,
            RF::UpdateFrequency::PerEssence,
            mGraphicDescriptorSet
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindVertexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindVertexBuffer(recordState, *mGpuModel->meshBuffers->vertexBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugEssence::bindIndexBuffer(RT::CommandRecordState const & recordState) const
    {
        RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indexBuffer);
    }

    //-------------------------------------------------------------------------------------------------

}
