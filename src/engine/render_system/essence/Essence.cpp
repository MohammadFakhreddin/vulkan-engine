#include "Essence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::Essence::Essence(std::shared_ptr<RT::GpuModel> gpuModel)
    : mGpuModel(std::move(gpuModel))
{}

//-------------------------------------------------------------------------------------------------

MFA::Essence::~Essence() = default;

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::GpuModelId MFA::Essence::GetId() const
{
    return mGpuModel->id;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel * MFA::Essence::GetGpuModel() const {
    return mGpuModel.get();
}

//-------------------------------------------------------------------------------------------------
  
MFA::RT::DescriptorSetGroup const & MFA::Essence::CreateDescriptorSetGroup(
    const VkDescriptorPool descriptorPool,
    uint32_t const descriptorSetCount,
    const VkDescriptorSetLayout descriptorSetLayout
)
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == false);
    mDescriptorSetGroup = RF::CreateDescriptorSets(
        descriptorPool,
        descriptorSetCount,
        descriptorSetLayout
    );
    mIsDescriptorSetGroupValid = true;
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DescriptorSetGroup const & MFA::Essence::GetDescriptorSetGroup() const
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

void MFA::Essence::BindVertexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindVertexBuffer(recordState, *mGpuModel->meshBuffers->verticesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::Essence::BindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indicesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::Essence::BindDescriptorSetGroup(RT::CommandRecordState const & recordState) const
{
    RF::BindDescriptorSet(
        recordState,
        RenderFrontend::DescriptorSetType::PerEssence,
        mDescriptorSetGroup
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::Essence::BindAllRenderRequiredData(RT::CommandRecordState const & recordState) const
{
    BindDescriptorSetGroup(recordState);
    BindVertexBuffer(recordState);
    BindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------
