#include "EssenceBase.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::EssenceBase::EssenceBase(std::shared_ptr<RT::GpuModel> gpuModel)
    : mGpuModel(std::move(gpuModel))
{}

//-------------------------------------------------------------------------------------------------

MFA::EssenceBase::~EssenceBase() = default;

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::GpuModelId MFA::EssenceBase::GetId() const
{
    return mGpuModel->id;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel * MFA::EssenceBase::GetGpuModel() const {
    return mGpuModel.get();
}

//-------------------------------------------------------------------------------------------------
  
MFA::RT::DescriptorSetGroup const & MFA::EssenceBase::CreateDescriptorSetGroup(
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

MFA::RT::DescriptorSetGroup const & MFA::EssenceBase::GetDescriptorSetGroup() const
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::BindVertexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindVertexBuffer(recordState, *mGpuModel->meshBuffers->verticesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::BindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indicesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::BindDescriptorSetGroup(RT::CommandRecordState const & recordState) const
{
    RF::BindDescriptorSet(
        recordState,
        RenderFrontend::DescriptorSetType::PerEssence,
        mDescriptorSetGroup
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::BindAllRenderRequiredData(RT::CommandRecordState const & recordState) const
{
    BindDescriptorSetGroup(recordState);
    BindVertexBuffer(recordState);
    BindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------
