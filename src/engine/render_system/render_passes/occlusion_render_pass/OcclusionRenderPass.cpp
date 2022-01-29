#include "OcclusionRenderPass.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/RenderBackend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::OcclusionRenderPass::OcclusionRenderPass() = default;

//-------------------------------------------------------------------------------------------------

VkRenderPass MFA::OcclusionRenderPass::GetVkRenderPass()
{
    return mRenderPass;
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalInit()
{
    auto const capabilities = RF::GetSurfaceCapabilities();
    mImageWidth = capabilities.currentExtent.width;
    mImageHeight = capabilities.currentExtent.height;

    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    createRenderPass();

    createFrameBuffers(extent);
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalShutdown()
{
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()),
        mFrameBuffers.data()
    );

    RF::DestroyRenderPass(mRenderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::BeginRenderPass(RT::CommandRecordState & recordState)
{
    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    //copyDisplayPassDepthBuffer(recordState, extent);

    RenderPass::BeginRenderPass(recordState);

    RF::AssignViewportAndScissorToCommandBuffer(RF::GetGraphicCommandBuffer(recordState), extent);

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(1);
    clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0 };

    RF::BeginRenderPass(
        RF::GetGraphicCommandBuffer(recordState),
        mRenderPass,
        getFrameBuffer(recordState),
        extent,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::EndRenderPass(RT::CommandRecordState & recordState)
{
    RenderPass::EndRenderPass(recordState);
    RF::EndRenderPass(RF::GetGraphicCommandBuffer(recordState));
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::OnResize()
{
    auto const capabilities = RF::GetSurfaceCapabilities();
    mImageWidth = capabilities.currentExtent.width;
    mImageHeight = capabilities.currentExtent.height;

    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    //createDepthImage(extent);

    // Depth frame-buffer
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()),
        mFrameBuffers.data()
    );
    createFrameBuffers(extent);
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::createRenderPass()
{

    VkAttachmentDescription const depthAttachment{
        .format = RF::GetDisplayRenderPass()->GetDepthImages()[0]->imageFormat,//mDepthImageGroupList[0]->imageFormat,
        .samples = RF::GetMaxSamplesCount(),
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, //VK_ATTACHMENT_STORE_OP_DONT_CARE
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    std::vector<VkSubpassDescription> subPassDescription{
        VkSubpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 0,
            .pDepthStencilAttachment = &depthAttachmentRef,
        }
    };

    std::vector<VkAttachmentDescription> attachments{ depthAttachment };

    mRenderPass = RF::CreateRenderPass(
        attachments.data(),
        static_cast<uint32_t>(attachments.size()),
        subPassDescription.data(),
        static_cast<uint32_t>(subPassDescription.size()),
        nullptr,
        0
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::createFrameBuffers(VkExtent2D const & extent)
{
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetSwapChainImagesCount());

    for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments {
            //mDepthImageGroupList[i]->imageView->imageView
            RF::GetDisplayRenderPass()->GetDepthImages()[i]->imageView->imageView
        };
        mFrameBuffers[i] = RF::CreateFrameBuffer(
            mRenderPass,
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            extent,
            1
        );
    }
}

//-------------------------------------------------------------------------------------------------

//void MFA::OcclusionRenderPass::createDepthImage(VkExtent2D const & extent)
//{
//    mDepthImageGroupList.resize(RF::GetSwapChainImagesCount());
//    for (auto & depthImage : mDepthImageGroupList)
//    {
//        depthImage = RF::CreateDepthImage(
//            extent,
//            RT::CreateDepthImageOptions{
//                .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
//                .samplesCount = RF::GetMaxSamplesCount()
//            }
//        );
//    }
//}

//-------------------------------------------------------------------------------------------------

//void MFA::OcclusionRenderPass::copyDisplayPassDepthBuffer(
//    RT::CommandRecordState const & recordState,
//    VkExtent2D const & extent2D
//) const
//{
//    // TODO Try readonly depth buffer as well. Maybe we can avoid having extra depth buffer and copy process
//    auto const & displayPassDepthImage = RF::GetDisplayRenderPass()->GetDepthImages()[recordState.imageIndex]->imageGroup->image;
//    auto const & occlusionPassDepthImage = mDepthImageGroupList[recordState.imageIndex]->imageGroup->image;
//
//    {// Preparing images for copy
//        std::vector<VkImageMemoryBarrier> memoryBarriers {};
//
//        VkImageSubresourceRange const subResourceRange{
//            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
//            .baseMipLevel = 0,
//            .levelCount = 1,
//            .baseArrayLayer = 0,
//            .layerCount = 1,
//        };
//
//        memoryBarriers.emplace_back(VkImageMemoryBarrier {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .srcAccessMask = 0,
//            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
//            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .image = displayPassDepthImage,
//            .subresourceRange = subResourceRange
//        });
//
//        memoryBarriers.emplace_back(VkImageMemoryBarrier {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .srcAccessMask = 0,
//            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .image = occlusionPassDepthImage,
//            .subresourceRange = subResourceRange
//        });
//
//        RF::PipelineBarrier(
//            RF::GetGraphicCommandBuffer(recordState),
//            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
//            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
//            static_cast<uint32_t>(memoryBarriers.size()),
//            memoryBarriers.data()
//        );
//    }
//    {// Copy
//        // Copy region for transfer from frame buffer to cube face
//        VkImageCopy const copyRegion{
//            .srcSubresource = {
//                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
//                .mipLevel = 0,
//                .baseArrayLayer = 0,
//                .layerCount = 1,
//            },
//            .srcOffset = { 0, 0, 0 },
//            .dstSubresource {
//                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
//                .mipLevel = 0,
//                .baseArrayLayer = 0,
//                .layerCount = 1,
//            },
//            .dstOffset = { 0, 0, 0 },
//            .extent {
//                .width = extent2D.width,
//                .height = extent2D.height,
//                .depth = 1,
//            }
//        };
//        RB::CopyImage(
//            RF::GetGraphicCommandBuffer(recordState),
//            displayPassDepthImage,
//            occlusionPassDepthImage,
//            copyRegion
//        );
//    }
//    {// Restoring images to their original state
//        std::vector<VkImageMemoryBarrier> memoryBarriers {};
//
//        VkImageSubresourceRange const subResourceRange{
//            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
//            .baseMipLevel = 0,
//            .levelCount = 1,
//            .baseArrayLayer = 0,
//            .layerCount = 1,
//        };
//
//        memoryBarriers.emplace_back(VkImageMemoryBarrier {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .srcAccessMask = 0,
//            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .image = displayPassDepthImage,
//            .subresourceRange = subResourceRange
//        });
//
//        memoryBarriers.emplace_back(VkImageMemoryBarrier {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .srcAccessMask = 0,
//            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//            .image = occlusionPassDepthImage,
//            .subresourceRange = subResourceRange
//        });
//
//        RF::PipelineBarrier(
//            RF::GetGraphicCommandBuffer(recordState),
//            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
//            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
//            static_cast<uint32_t>(memoryBarriers.size()),
//            memoryBarriers.data()
//        );
//    }
//}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::OcclusionRenderPass::getFrameBuffer(RT::CommandRecordState const & drawPass) const
{
    return mFrameBuffers[drawPass.imageIndex];
}

//-------------------------------------------------------------------------------------------------
