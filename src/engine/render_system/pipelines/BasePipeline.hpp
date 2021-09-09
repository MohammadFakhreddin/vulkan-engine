#pragma once

#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
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
    ) {}

    virtual void Render(        
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) = 0;

    virtual void PostRender(
        RF::DrawPass & drawPass,
        float deltaTime,
        uint32_t idsCount, 
        DrawableObjectId * ids
    ) {}

    virtual void CreateDrawableEssence(
        char const * name, 
        RF::GpuModel const & gpuModel, 
        DrawableEssenceId & outEssenceId
    ) = 0;

    virtual bool RemoveDrawableEssence(char const * name) = 0;

    virtual DrawableVariant * CreateDrawableVariant(char const * name) = 0;

    virtual void RemoveDrawableVariant(DrawableVariant * variant) = 0;
    
    virtual void OnResize() = 0;

private:

    std::unordered_map<std::string, std::unique_ptr<DrawableEssence>> mDrawableEssenceMap {};

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

    
    void AddCombinedImageSampler(VkDescriptorImageInfo * imageInfoList, uint32_t imageInfoCount, uint32_t offset = 0) {
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
    
    void AddSampler(VkDescriptorImageInfo const & samplerInfo, uint32_t offset = 0) {
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
    
    void AddImage(VkDescriptorImageInfo const * imageInfoList, uint32_t imageInfoCount, uint32_t offset = 0) {
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
