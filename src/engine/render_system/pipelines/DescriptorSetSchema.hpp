#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <cstdint>

namespace MFA {

class DescriptorSetSchema {
public:

    explicit DescriptorSetSchema(VkDescriptorSet descriptorSet);

    void AddUniformBuffer(VkDescriptorBufferInfo const & bufferInfo, uint32_t offset = 0);

    void AddCombinedImageSampler(VkDescriptorImageInfo const & imageInfo, uint32_t offset = 0);

    
    void AddCombinedImageSampler(
        VkDescriptorImageInfo * imageInfoList, 
        uint32_t imageInfoCount, 
        uint32_t offset = 0
    );
    
    void AddSampler(VkDescriptorImageInfo const & samplerInfo, uint32_t offset = 0);
    
    void AddImage(VkDescriptorImageInfo const * imageInfoList, uint32_t imageInfoCount, uint32_t offset = 0);

    void UpdateDescriptorSets();

private:

    bool isActive = false;
    VkDescriptorSet mDescriptorSet {};
    std::vector<VkWriteDescriptorSet> mWriteInfo {};

};

}
