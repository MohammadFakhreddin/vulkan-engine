#include "ShadowRenderPassV2.hpp"

#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderBackend.hpp"

namespace MFA {

VkRenderPass ShadowRenderPassV2::GetVkRenderPass() {
    return mVkRenderPass;
}

RT::DepthImageGroup const & ShadowRenderPassV2::GetDepthCubeMap() const {
    return mDepthCubeMap;
}

void ShadowRenderPassV2::internalInit() {
    MFA_ASSERT(mImageWidth > 0);
    MFA_ASSERT(mImageHeight > 0);

    auto const shadowExtend = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    mDepthCubeMap = RF::CreateDepthImage(shadowExtend, RT::CreateDepthImageOptions {
        .layerCount = 6, // * light count
        .usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    });

    mDepthImage = RF::CreateDepthImage(shadowExtend, RT::CreateDepthImageOptions {
        .layerCount = 1,
        .usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    });

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

void ShadowRenderPassV2::internalShutdown() {
    RF::DestroyFrameBuffers(1, &mFrameBuffer);
    RF::DestroyRenderPass(mVkRenderPass);
    RF::DestroyDepthImage(mDepthCubeMap);
    RF::DestroyDepthImage(mDepthImage);
}

void ShadowRenderPassV2::SetNextPassParams(int faceIndex) {
    MFA_ASSERT(getIsRenderPassActive() == false);
    mFaceIndex = faceIndex;
}

void ShadowRenderPassV2::PrepareCubeMapForTransferDestination(RT::DrawPass const & drawPass) {
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
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mDepthCubeMap.imageGroup.image,
        .subresourceRange = subResourceRange
    };
    
    RF::PipelineBarrier(
        RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        pipelineBarrier
    );
}

void ShadowRenderPassV2::PrepareCubeMapForSampling(RT::DrawPass const & drawPass) {
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
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mDepthCubeMap.imageGroup.image,
        .subresourceRange = subResourceRange
    };
    
    RF::PipelineBarrier(
        RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        pipelineBarrier
    );
}

void ShadowRenderPassV2::internalBeginRenderPass(RT::DrawPass & drawPass) {

    {// Making depth image ready for depth attachment
        VkImageSubresourceRange const subResourceRange {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier const pipelineBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = mDepthImage.imageGroup.image,
            .subresourceRange = subResourceRange
        };
        
        RF::PipelineBarrier(
            RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            pipelineBarrier
        );
    }

    // Shadow map generation
    auto const shadowExtend = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    auto * commandBuffer = mDisplayRenderPass->GetCommandBuffer(drawPass);
    MFA_ASSERT(commandBuffer != nullptr);

    RF::AssignViewportAndScissorToCommandBuffer(commandBuffer, shadowExtend);

    std::vector<VkClearValue> clearValues {};
    clearValues.resize(1);
    clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0};

    RF::BeginRenderPass(
        commandBuffer,
        mVkRenderPass,
        mFrameBuffer,
        shadowExtend,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );
}

void ShadowRenderPassV2::internalEndRenderPass(RT::DrawPass & drawPass) {
    MFA_ASSERT(mFaceIndex >= 0 && mFaceIndex < 6);
    RF::EndRenderPass(mDisplayRenderPass->GetCommandBuffer(drawPass));

    {// Depth image barrier
        VkImageSubresourceRange const subResourceRange {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier const pipelineBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = mDepthImage.imageGroup.image,
            .subresourceRange = subResourceRange
        };
        
        RF::PipelineBarrier(
            RF::GetDisplayRenderPass()->GetCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            pipelineBarrier
        );
    }

    {// Copy depth image into DepthCubeMap
        auto const shadowExtend = VkExtent2D {
            .width = mImageWidth,
            .height = mImageHeight
        };
        // Copy region for transfer from frame buffer to cube face
        VkImageCopy const copyRegion {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .mipLevel = 0,
                .baseArrayLayer = static_cast<uint32_t>(mFaceIndex),
                .layerCount = 1,
            },
            .dstOffset = { 0, 0, 0 },
            .extent {
                .width = shadowExtend.width,
                .height = shadowExtend.height,
                .depth = 1,
            }
        };
        RB::CopyImage(
            mDisplayRenderPass->GetCommandBuffer(drawPass), 
            mDepthImage.imageGroup.image, 
            mDepthCubeMap.imageGroup.image, 
            copyRegion
        );
    }

    mFaceIndex = -1;
}

void ShadowRenderPassV2::internalResize() {
}

void ShadowRenderPassV2::createRenderPass() {
    std::vector<VkAttachmentDescription> attachments {};
    
    // Depth attachment
    attachments.emplace_back(VkAttachmentDescription {
        .format = mDepthImage.imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        
    });
    
    VkAttachmentReference depthReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    std::vector<VkSubpassDescription> subPasses {
        VkSubpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 0,
            .pDepthStencilAttachment = &depthReference,
        }
    };

    // TODO Are these dependencies correct ?
    // Subpass dependencies for layout transitions
    std::vector<VkSubpassDependency> dependencies {
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
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
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

void ShadowRenderPassV2::createFrameBuffer(VkExtent2D const & shadowExtent) {

    // Note: This comment is useful though does not match 100% with my code
    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    
    std::vector<VkImageView> const attachments = {
        mDepthImage.imageView
    };

    mFrameBuffer = RF::CreateFrameBuffer(
        mVkRenderPass, 
        attachments.data(), 
        static_cast<uint32_t>(attachments.size()),
        shadowExtent,
        1   // * lightCount
    );
}

}