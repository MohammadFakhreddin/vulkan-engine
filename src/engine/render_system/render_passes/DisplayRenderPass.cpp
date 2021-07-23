#include "DisplayRenderPass.hpp"

namespace MFA {

VkRenderPass DisplayRenderPass::GetVkRenderPass() {
    return mVkRenderPass;
}

VkCommandBuffer DisplayRenderPass::GetCommandBuffer(RF::DrawPass const & drawPass) {
    return mGraphicCommandBuffers[drawPass.imageIndex];
}

void DisplayRenderPass::internalInit() {

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtent = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    mSwapChainImagesCount = RF::GetSwapChainImagesCount();

    mSwapChainImages = RF::CreateSwapChain();

    mVkRenderPass = RF::CreateRenderPass(mSwapChainImages.swapChainFormat);

    mDepthImageGroup = RF::CreateDepthImage(swapChainExtent);

    createFrameBuffers(swapChainExtent);

    mGraphicCommandBuffers = RF::CreateGraphicCommandBuffers(mSwapChainImagesCount);

    mSyncObjects = RF::createSyncObjects(
        MAX_FRAMES_IN_FLIGHT,
        mSwapChainImagesCount
    );
}

void DisplayRenderPass::internalShutdown() {
    RF::DestroySyncObjects(mSyncObjects);

    RF::DestroyGraphicCommandBuffer(
        mGraphicCommandBuffers.data(),
        static_cast<uint32_t>(mGraphicCommandBuffers.size())
    );

    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()), 
        mFrameBuffers.data()
    );

    RF::DestroyDepthImage(mDepthImageGroup);

    RF::DestroyRenderPass(mVkRenderPass);

    RF::DestroySwapChain(mSwapChainImages);
}

RF::DrawPass DisplayRenderPass::internalBegin() {
    MFA_ASSERT(MAX_FRAMES_IN_FLIGHT > mCurrentFrame);
    RF::DrawPass drawPass {.renderPass = this};
    if (RF::IsWindowVisible() == false || RF::IsWindowResized() == true) {
        drawPass.isValid = false;
        return drawPass;
    }


    drawPass.frameIndex = mCurrentFrame;
    drawPass.isValid = true;
    ++mCurrentFrame;
    if(mCurrentFrame >= MAX_FRAMES_IN_FLIGHT) {
        mCurrentFrame = 0;
    }

    RF::WaitForFence(getInFlightFence(drawPass));

    // TODO Maybe we should care for failed image acquire
    // We ignore failed acquire of image because a resize will be triggered at end of pass
    RF::AcquireNextImage(
        getImageAvailabilitySemaphore(drawPass),
        mSwapChainImages,
        drawPass.imageIndex
    );

    // Recording command buffer data at each render frame
    // We need 1 renderPass and multiple command buffer recording
    // Each pipeline has its own set of shader, But we can reuse a pipeline for multiple shaders.
    // For each model we need to record command buffer with our desired pipeline (For example light and objects have different fragment shader)
    // Prepare data for recording command buffers
    RF::BeginCommandBuffer(GetCommandBuffer(drawPass));

    // If present queue family and graphics queue family are different, then a barrier is necessary
    // The barrier is also needed initially to transition the image to the present layout
    VkImageMemoryBarrier presentToDrawBarrier = {};
    presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentToDrawBarrier.srcAccessMask = 0;
    presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    auto const presentQueueFamily = RF::GetPresentQueueFamily();
    auto const graphicQueueFamily = RF::GetGraphicQueueFamily();

    if (presentQueueFamily != graphicQueueFamily) {
        presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else {
        presentToDrawBarrier.srcQueueFamilyIndex = presentQueueFamily;
        presentToDrawBarrier.dstQueueFamilyIndex = graphicQueueFamily;
    }

    presentToDrawBarrier.image = mSwapChainImages.swapChainImages[drawPass.imageIndex];

    VkImageSubresourceRange const subResourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    presentToDrawBarrier.subresourceRange = subResourceRange;
    
    RF::PipelineBarrier(
        GetCommandBuffer(drawPass),
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        presentToDrawBarrier
    );

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    RF::AssignViewportAndScissorToCommandBuffer(GetCommandBuffer(drawPass), swapChainExtend);

    RF::BeginRenderPass(
        GetCommandBuffer(drawPass), 
        mVkRenderPass, 
        getFrameBuffer(drawPass)
    );

    return drawPass;
}

void DisplayRenderPass::internalEnd(RF::DrawPass & drawPass) {
    if (RF::IsWindowVisible() == false) {
        return;
    }

    RF::EndRenderPass(GetCommandBuffer(drawPass));
    
    MFA_ASSERT(drawPass.isValid);
    drawPass.isValid = false;


    auto const presentQueueFamily = RF::GetPresentQueueFamily();
    auto const graphicQueueFamily = RF::GetGraphicQueueFamily();

    // If present and graphics queue families differ, then another barrier is required
    if (presentQueueFamily != graphicQueueFamily) {
        // TODO Check that WTF is this ?
        VkImageSubresourceRange const subResourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier drawToPresentBarrier = {};
        drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        drawToPresentBarrier.srcQueueFamilyIndex = graphicQueueFamily;
        drawToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
        drawToPresentBarrier.image = getSwapChainImage(drawPass);
        drawToPresentBarrier.subresourceRange = subResourceRange;

        RF::PipelineBarrier(
            GetCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            drawToPresentBarrier
        );
    }

    RF::EndCommandBuffer(GetCommandBuffer(drawPass));
    
    // Wait for image to be available and draw
    RF::SubmitQueue(
        GetCommandBuffer(drawPass),
        getImageAvailabilitySemaphore(drawPass),
        getRenderFinishIndicatorSemaphore(drawPass),
        getInFlightFence(drawPass)
    );
    
    // Present drawn image
    RF::PresentQueue(
        drawPass.imageIndex, 
        getRenderFinishIndicatorSemaphore(drawPass), 
        mSwapChainImages.swapChain
    );
}

void DisplayRenderPass::internalResize() {

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    auto const swapChainExtent2D = VkExtent2D {
        .width = swapChainExtend.width,
        .height = swapChainExtend.height
    };

    // Depth image
    RF::DestroyDepthImage(mDepthImageGroup);
    mDepthImageGroup = RF::CreateDepthImage(swapChainExtent2D);

    // Swap-chain
    auto const oldSwapChainImages = mSwapChainImages;
    mSwapChainImages = RF::CreateSwapChain();
    RF::DestroySwapChain(oldSwapChainImages);
    
    // Frame-buffer
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()), 
        mFrameBuffers.data()
    );
    createFrameBuffers(swapChainExtent2D);

    // Command buffer
    RF::DestroyGraphicCommandBuffer(
        mGraphicCommandBuffers.data(), 
        static_cast<uint32_t>(mGraphicCommandBuffers.size())
    );
    mGraphicCommandBuffers = RF::CreateGraphicCommandBuffers(mSwapChainImagesCount);

}

VkSemaphore DisplayRenderPass::getImageAvailabilitySemaphore(RF::DrawPass const & drawPass) {
    return mSyncObjects.imageAvailabilitySemaphores[drawPass.frameIndex];
}

VkSemaphore DisplayRenderPass::getRenderFinishIndicatorSemaphore(RF::DrawPass const & drawPass) {
    return mSyncObjects.renderFinishIndicatorSemaphores[drawPass.frameIndex];    
}

VkFence DisplayRenderPass::getInFlightFence(RF::DrawPass const & drawPass) {
    return mSyncObjects.fencesInFlight[drawPass.frameIndex];
}

VkImage DisplayRenderPass::getSwapChainImage(RF::DrawPass const & drawPass) {
    return mSwapChainImages.swapChainImages[drawPass.imageIndex];
}

VkFramebuffer DisplayRenderPass::getFrameBuffer(RF::DrawPass const & drawPass) {
    return mFrameBuffers[drawPass.imageIndex];
}

void DisplayRenderPass::createFrameBuffers(VkExtent2D const & extent) {
    mFrameBuffers.erase(mFrameBuffers.begin(), mFrameBuffers.end());
    mFrameBuffers.resize(mSwapChainImagesCount);
    for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i) {
        std::vector<VkImageView> const attachments = {
            mSwapChainImages.swapChainImageViews[i],
            mDepthImageGroup.imageView
        };
        mFrameBuffers[i] = RF::CreateFrameBuffer(
            mVkRenderPass, 
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            extent
        );
    }
}

}
