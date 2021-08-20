#pragma once

#include "engine/render_system/DrawableObject.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RB = RenderBackend;
namespace RF = RenderFrontend;

class BasePipeline {
public:
    
    virtual ~BasePipeline() = default;

    virtual void PreRender(
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) {};

    virtual void Render(        
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) = 0;

    virtual DrawableObjectId AddGpuModel(RF::GpuModel & gpuModel) = 0;

    virtual bool RemoveGpuModel(DrawableObjectId drawableObjectId) = 0;

    virtual void OnResize() = 0;

};

class DescriptorSetSchema {
public:

    explicit DescriptorSetSchema(VkDescriptorSet descriptorSet)
        : mDescriptorSet(descriptorSet)
    {
        MFA_VK_VALID_ASSERT(descriptorSet);
        isActive = true;
    }

    void AddUniformBuffer(VkDescriptorBufferInfo const & bufferInfo, uint32_t offset = 0) {
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

    void AddCombinedImageSampler(VkDescriptorImageInfo const & imageInfo, uint32_t offset = 0) {
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

    void UpdateDescriptorSets() {
        isActive = false;
        RF::UpdateDescriptorSets(
            static_cast<uint32_t>(mWriteInfo.size()), 
            mWriteInfo.data()
        );
    }

private:

    bool isActive = false;
    VkDescriptorSet mDescriptorSet {};
    std::vector<VkWriteDescriptorSet> mWriteInfo {};

};

}