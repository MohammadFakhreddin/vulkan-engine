#include "DisplayRenderPass.hpp"

namespace MFA {

VkRenderPass DisplayRenderPass::GetVkRenderPass() {
    return mVkRenderPass;
}

void DisplayRenderPass::internalInit() {

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    mSwapChainImages = RF::CreateSwapChain();
    mVkRenderPass = RF::CreateRenderPass(mSwapChainImages.swapChainFormat);
    mDepthImageGroup = RF::CreateDepthImage(swapChainExtend);
    mFrameBuffers = RF::CreateFrameBuffers(
        mVkRenderPass, 
        static_cast<uint32_t>(mSwapChainImages.swapChainImageViews.size()), 
        mSwapChainImages.swapChainImageViews.data(),
        mDepthImageGroup.imageView,
        swapChainExtend
    );
}

void DisplayRenderPass::internalShutdown() {
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()), 
        mFrameBuffers.data()
    );
    RF::DestroyDepthImage(mDepthImageGroup);
    RF::DestroyRenderPass(mVkRenderPass);
    RF::DestroySwapChain(mSwapChainImages);
}

RF::DrawPass DisplayRenderPass::internalBegin() {
    MFA_ASSERT(RF::MAX_FRAMES_IN_FLIGHT > mCurrentFrame);
    RF::DrawPass drawPass {};
    if (RF::IsWindowVisible() == false || RF::IsWindowResized() == true) {
        RF::DrawPass drawPass {};
        drawPass.isValid = false;
        return drawPass;
    }

    RF::WaitForFence(mCurrentFrame);

    // TODO Maybe we should care for failed image acquire
    // We ignore failed acquire of image because a resize will be triggered at end of pass
    RF::AcquireNextImage(
        mCurrentFrame,
        mSwapChainImages,
        drawPass.imageIndex
    );

    drawPass.frameIndex = mCurrentFrame;
    drawPass.isValid = true;
    ++mCurrentFrame;
    if(mCurrentFrame >= RF::MAX_FRAMES_IN_FLIGHT) {
        mCurrentFrame = 0;
    }

    // Recording command buffer data at each render frame
    // We need 1 renderPass and multiple command buffer recording
    // Each pipeline has its own set of shader, But we can reuse a pipeline for multiple shaders.
    // For each model we need to record command buffer with our desired pipeline (For example light and objects have different fragment shader)
    // Prepare data for recording command buffers
    RF::BeginCommandBuffer(drawPass.imageIndex);

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
        drawPass.imageIndex,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        presentToDrawBarrier
    );
    
    RF::AssignViewportAndScissorToCommandBuffer(drawPass.imageIndex);

    RF::BeginRenderPass(drawPass, mVkRenderPass, mFrameBuffers.data());

    return drawPass;
}

void DisplayRenderPass::internalEnd(RF::DrawPass & drawPass) {
    if (RF::IsWindowVisible() == false) {
        return;
    }

    RF::EndRenderPass(drawPass);
    
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
        drawToPresentBarrier.image = mSwapChainImages.swapChainImages[drawPass.imageIndex];
        drawToPresentBarrier.subresourceRange = subResourceRange;

        RF::PipelineBarrier(
            drawPass.imageIndex,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            drawToPresentBarrier
        );
    }

    RF::EndCommandBuffer(drawPass.imageIndex);
    
    // Wait for image to be available and draw
    //VkSubmitInfo submitInfo = {};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //VkSemaphore wait_semaphores[] = {state->syncObjects.image_availability_semaphores[drawPass.frameIndex]};
    //VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    //submitInfo.waitSemaphoreCount = 1;
    //submitInfo.pWaitSemaphores = wait_semaphores;
    //submitInfo.pWaitDstStageMask = wait_stages;

    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &state->graphicCommandBuffers[drawPass.imageIndex];

    //VkSemaphore signal_semaphores[] = {state->syncObjects.render_finish_indicator_semaphores[drawPass.frameIndex]};
    //submitInfo.signalSemaphoreCount = 1;
    //submitInfo.pSignalSemaphores = signal_semaphores;

    //vkResetFences(state->logicalDevice.device, 1, &state->syncObjects.fences_in_flight[drawPass.frameIndex]);

    //auto const result = vkQueueSubmit(
    //    state->graphicQueue, 
    //    1, &submitInfo, 
    //    state->syncObjects.fences_in_flight[drawPass.frameIndex]
    //);
    //if (result != VK_SUCCESS) {
    //    MFA_CRASH("Failed to submit draw command buffer");
    //}
    RF::SubmitQueue(drawPass.imageIndex, drawPass.frameIndex);
    
    // Present drawn image
    // Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
    //VkPresentInfoKHR presentInfo = {};
    //presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //presentInfo.waitSemaphoreCount = 1;
    //presentInfo.pWaitSemaphores = signal_semaphores;
    //VkSwapchainKHR swapChains[] = {state->swapChainGroup.swapChain};
    //presentInfo.swapchainCount = 1;
    //presentInfo.pSwapchains = swapChains;
    //uint32_t imageIndices = drawPass.imageIndex;
    //presentInfo.pImageIndices = &imageIndices;

    //auto const res = vkQueuePresentKHR(state->presentQueue, &presentInfo);
    //// TODO Handle resize event
    //if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || state->windowResized == true) {
    //    OnWindowResized();
    //} else if (res != VK_SUCCESS) {
    //    MFA_CRASH("Failed to submit present command buffer");
    //}
    RF::PresentQueue(drawPass.imageIndex, drawPass.frameIndex, mSwapChainImages.swapChain);
}

void DisplayRenderPass::internalResize() {

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    auto const extent2D = VkExtent2D {
        .width = swapChainExtend.width,
        .height = swapChainExtend.height
    };

    // Depth image
    RF::DestroyDepthImage(mDepthImageGroup);
    mDepthImageGroup = RF::CreateDepthImage(extent2D);

    // Swap-chain
    auto const oldSwapChainImages = mSwapChainImages;
    mSwapChainImages = RF::CreateSwapChain();
    RF::DestroySwapChain(oldSwapChainImages);
    
    // Frame-buffer
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()), 
        mFrameBuffers.data()
    );
    mFrameBuffers = RF::CreateFrameBuffers(
        mVkRenderPass, 
        static_cast<uint32_t>(mSwapChainImages.swapChainImageViews.size()), 
        mSwapChainImages.swapChainImageViews.data(),
        mDepthImageGroup.imageView,
        swapChainExtend
    );

}

}
