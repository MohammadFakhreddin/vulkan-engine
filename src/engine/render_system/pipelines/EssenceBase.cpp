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

std::string const & MFA::EssenceBase::getNameOrAddress() const
{
    return mGpuModel->address;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::GpuModel * MFA::EssenceBase::getGpuModel() const {
    return mGpuModel.get();
}

//-------------------------------------------------------------------------------------------------
  
MFA::RT::DescriptorSetGroup const & MFA::EssenceBase::createDescriptorSetGroup(
    VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount,
    RT::DescriptorSetLayoutGroup const & descriptorSetLayoutGroup
)
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == false);
    mDescriptorSetGroup = RF::CreateDescriptorSets(
        descriptorPool,
        descriptorSetCount,
        descriptorSetLayoutGroup
    );
    mIsDescriptorSetGroupValid = true;
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DescriptorSetGroup const & MFA::EssenceBase::getDescriptorSetGroup() const
{
    MFA_ASSERT(mIsDescriptorSetGroupValid == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::bindIndexBuffer(RT::CommandRecordState const & recordState) const
{
    RF::BindIndexBuffer(recordState, *mGpuModel->meshBuffers->indicesBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::bindDescriptorSetGroup(RT::CommandRecordState const & recordState) const
{
    RF::BindDescriptorSet(
        recordState,
        RenderFrontend::DescriptorSetType::PerEssence,
        mDescriptorSetGroup
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::EssenceBase::bindAllRenderRequiredData(RT::CommandRecordState const & recordState) const
{
    bindDescriptorSetGroup(recordState);
    bindVertexBuffer(recordState);
    bindIndexBuffer(recordState);
}

//-------------------------------------------------------------------------------------------------
