#include "OffScreenRenderPass.hpp"

#include "DisplayRenderPass.hpp"

namespace MFA {

VkRenderPass OffScreenRenderPass::GetVkRenderPass() {
    return mVkRenderPass;
}

void OffScreenRenderPass::internalInit() {
    MFA_ASSERT(mImageWidth > 0);
    MFA_ASSERT(mImageHeight > 0);

    auto const shadowExtend = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    // TODO We might need to change usageFlags: VK_IMAGE_USAGE_TRANSFER_SRC_BIT feels extra
    mDepthImageGroup = RF::CreateDepthImage(shadowExtend, RB::CreateDepthImageOptions {
        .layerCount = 6, // * light count
        .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT// | VK_IMAGE_USAGE_SAMPLED_BIT
    });

    mColorImageGroup = RF::CreateColorImage(
        shadowExtend,
        SHADOW_MAP_FORMAT,
        RB::CreateColorImageOptions {
            .layerCount = 6, // * light count
            .usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        }
    );

    createRenderPass();
    
    createFrameBuffer(shadowExtend);

    // TODO We might need special sampler as well
    // Create sampler
    /*
    VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = cubeMap.mipLevels;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.maxAnisotropy = 1.0f;
    if (vulkanDevice->features.samplerAnisotropy)
    {
        sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
        sampler.anisotropyEnable = VK_TRUE;
    }
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &cubeMap.sampler));
    */

    mDisplayRenderPass = RF::GetDisplayRenderPass();
}

void OffScreenRenderPass::internalShutdown() {
    RF::DestroyFrameBuffers(1, &mFrameBuffer);
    RF::DestroyRenderPass(mVkRenderPass);
    RF::DestroyColorImage(mColorImageGroup);
    RF::DestroyDepthImage(mDepthImageGroup);
}

void OffScreenRenderPass::internalBeginRenderPass(RF::DrawPass const & drawPass) {
    // TODO What does pipeline barrier do exactly ?
    {// Color barrier
        VkImageSubresourceRange const subResourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 6,
        };

        VkImageMemoryBarrier const pipelineBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = mColorImageGroup.imageGroup.image,
            .subresourceRange = subResourceRange
        };
        
        RF::PipelineBarrier(
            RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            pipelineBarrier
        );
    }
    {// Depth barrier
        VkImageSubresourceRange const subResourceRange {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 6,
        };

        VkImageMemoryBarrier const pipelineBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = mDepthImageGroup.imageGroup.image,
            .subresourceRange = subResourceRange
        };
        
        RF::PipelineBarrier(
            RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            pipelineBarrier
        );
    }

    //  Shadow map generation
    auto const shadowExtend = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    auto * commandBuffer = mDisplayRenderPass->GetCommandBuffer(drawPass);
    MFA_ASSERT(commandBuffer != nullptr);

    RF::AssignViewportAndScissorToCommandBuffer(commandBuffer, shadowExtend);

    RF::BeginRenderPass(
        commandBuffer,
        mVkRenderPass,
        mFrameBuffer,
        shadowExtend
    );
}

void OffScreenRenderPass::internalEndRenderPass(RF::DrawPass const & drawPass) {
    RF::EndRenderPass(mDisplayRenderPass->GetCommandBuffer(drawPass));
}

void OffScreenRenderPass::internalResize() {
}

void OffScreenRenderPass::createRenderPass() {
    std::vector<VkAttachmentDescription> attachments {};

    // Color attachment
    attachments.emplace_back(VkAttachmentDescription {
        .format = SHADOW_MAP_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    });
    
    // Depth attachment
    attachments.emplace_back(VkAttachmentDescription {
        .format = mDepthImageGroup.imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    });
    
    VkAttachmentReference colorReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthReference {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    
    std::vector<VkSubpassDescription> subPasses {
        VkSubpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorReference,
            .pDepthStencilAttachment = &depthReference,
        }
    };

    // TODO Are these dependencies correct ?
    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies {
        VkSubpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        VkSubpassDependency {
            dependencies[1].srcSubpass = 0,
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL,
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        }
    };

    mVkRenderPass = RF::CreateRenderPass(
        attachments.data(),
        static_cast<uint32_t>(attachments.size()),
        subPasses.data(),
        static_cast<uint32_t>(subPasses.size()),
        dependencies.data(),
        static_cast<uint32_t>(dependencies.size())
    );
}

void OffScreenRenderPass::createFrameBuffer(VkExtent2D const & shadowExtent) {

    // Note: This comment is useful though does not match 100% with my code
    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    
    std::vector<VkImageView> const attachments = {
        mColorImageGroup.imageView,
        mDepthImageGroup.imageView
    };

    mFrameBuffer = RF::CreateFrameBuffer(
        mVkRenderPass, 
        attachments.data(), 
        static_cast<uint32_t>(attachments.size()),
        shadowExtent,
        6   // * lightCount
    );
}

}
