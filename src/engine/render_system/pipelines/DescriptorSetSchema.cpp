#include "DescriptorSetSchema.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

DescriptorSetSchema::DescriptorSetSchema(VkDescriptorSet descriptorSet)
    : mDescriptorSet(descriptorSet)
{
    MFA_VK_VALID_ASSERT(descriptorSet);
    isActive = true;
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddUniformBuffer(VkDescriptorBufferInfo const & bufferInfo, uint32_t const offset) {
    MFA_ASSERT(isActive);

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddCombinedImageSampler(VkDescriptorImageInfo const & imageInfo, uint32_t const offset) {
    MFA_ASSERT(isActive);

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddCombinedImageSampler(
    VkDescriptorImageInfo * imageInfoList, 
    uint32_t imageInfoCount, 
    uint32_t offset
) {
    MFA_ASSERT(isActive);

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = imageInfoCount,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = imageInfoList,
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddSampler(VkDescriptorImageInfo const & samplerInfo, uint32_t const offset) {
    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &samplerInfo,
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddImage(
    VkDescriptorImageInfo const * imageInfoList, 
    uint32_t const imageInfoCount, 
    uint32_t const offset
) {
    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = imageInfoCount,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = imageInfoList
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::AddStorageBuffer(VkDescriptorBufferInfo const & bufferInfo, uint32_t offset)
{
    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = static_cast<uint32_t>(mWriteInfo.size()) + offset,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    mWriteInfo.emplace_back(writeDescriptorSet);
}

//-------------------------------------------------------------------------------------------------

void DescriptorSetSchema::UpdateDescriptorSets() {
    isActive = false;
    RF::UpdateDescriptorSets(
        static_cast<uint32_t>(mWriteInfo.size()),
        mWriteInfo.data()
    );
}

//-------------------------------------------------------------------------------------------------

}
