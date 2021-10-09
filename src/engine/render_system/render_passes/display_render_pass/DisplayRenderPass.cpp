#include "DisplayRenderPass.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    VkRenderPass DisplayRenderPass::GetVkRenderPass()
    {
        return mVkDisplayRenderPass;
    }

    //-------------------------------------------------------------------------------------------------

    VkRenderPass DisplayRenderPass::GetVkDepthRenderPass() const
    {
        return mDepthRenderPass;
    }

    //-------------------------------------------------------------------------------------------------

    VkCommandBuffer DisplayRenderPass::GetCommandBuffer(RT::DrawPass const & drawPass)
    {
        return mGraphicCommandBuffers[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalInit()
    {

        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtent = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        mSwapChainImagesCount = RF::GetSwapChainImagesCount();

        mSwapChainImages = RF::CreateSwapChain();

        mMSAAImageGroupList.resize(mSwapChainImagesCount);
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            mMSAAImageGroupList[i] = RF::CreateColorImage(
                swapChainExtent,
                mSwapChainImages.swapChainFormat,
                RT::CreateColorImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }

        mDepthImageGroupList.resize(mSwapChainImagesCount);
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            mDepthImageGroupList[i] = RF::CreateDepthImage(
                swapChainExtent,
                RT::CreateDepthImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }

        createDisplayRenderPass();

        createDepthRenderPass();

        createDisplayFrameBuffers(swapChainExtent);

        createDepthFrameBuffers(swapChainExtent);

        mGraphicCommandBuffers = RF::CreateGraphicCommandBuffers(RF::GetMaxFramesPerFlight());

        mSyncObjects = RF::createSyncObjects(
            RF::GetMaxFramesPerFlight(),
            mSwapChainImagesCount
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalShutdown()
    {
        RF::DestroySyncObjects(mSyncObjects);

        RF::DestroyGraphicCommandBuffer(
            mGraphicCommandBuffers.data(),
            static_cast<uint32_t>(mGraphicCommandBuffers.size())
        );

        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDisplayFrameBuffers.size()),
            mDisplayFrameBuffers.data()
        );

        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDepthFrameBuffers.size()),
            mDepthFrameBuffers.data()
        );

        RF::DestroyRenderPass(mVkDisplayRenderPass);

        RF::DestroyRenderPass(mDepthRenderPass);

        for (auto & depthImage : mDepthImageGroupList)
        {
            RF::DestroyDepthImage(depthImage);
        }
        mDepthImageGroupList.clear();

        RF::DestroySwapChain(mSwapChainImages);

        for (auto & msaaImage : mMSAAImageGroupList)
        {
            RF::DestroyColorImage(msaaImage);
        }
        mMSAAImageGroupList.clear();
    }

    //-------------------------------------------------------------------------------------------------

    // TODO We might need a separate class for commandBufferClass->GraphicCommandBuffer to begin and submit commandBuffer recording.
    // TODO Rename DrawPass to recordPass
    RT::DrawPass DisplayRenderPass::StartGraphicCommandBufferRecording()
    {
        MFA_ASSERT(RF::GetMaxFramesPerFlight() > mCurrentFrame);
        RT::DrawPass drawPass{ .renderPass = this };
        if (RF::IsWindowVisible() == false || RF::IsWindowResized() == true)
        {
            drawPass.isValid = false;
            return drawPass;
        }

        drawPass.frameIndex = mCurrentFrame;
        drawPass.isValid = true;
        ++mCurrentFrame;
        if (mCurrentFrame >= RF::GetMaxFramesPerFlight())
        {
            mCurrentFrame = 0;
        }

        RF::WaitForFence(getInFlightFence(drawPass));

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

        return drawPass;
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::EndGraphicCommandBufferRecording(RT::DrawPass & drawPass)
    {
        MFA_ASSERT(drawPass.isValid);
        drawPass.isValid = false;

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

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::BeginDepthPrePass(RT::DrawPass & drawPass)
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        RF::AssignViewportAndScissorToCommandBuffer(GetCommandBuffer(drawPass), swapChainExtend);

        std::vector<VkClearValue> clearValues{};
        clearValues.resize(1);
        clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0 };

        RF::BeginRenderPass(
            GetCommandBuffer(drawPass),
            mDepthRenderPass,
            getDepthFrameBuffer(drawPass),
            swapChainExtend,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::EndDepthPrePass(RT::DrawPass & drawPass)
    {
        RF::EndRenderPass(GetCommandBuffer(drawPass));
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalBeginRenderPass(RT::DrawPass & drawPass)
    {
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

        if (presentQueueFamily != graphicQueueFamily)
        {
            presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        else
        {
            presentToDrawBarrier.srcQueueFamilyIndex = presentQueueFamily;
            presentToDrawBarrier.dstQueueFamilyIndex = graphicQueueFamily;
        }

        presentToDrawBarrier.image = mSwapChainImages.swapChainImages[drawPass.imageIndex];

        VkImageSubresourceRange const subResourceRange{
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
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        RF::AssignViewportAndScissorToCommandBuffer(GetCommandBuffer(drawPass), swapChainExtend);

        std::vector<VkClearValue> clearValues{};
        clearValues.resize(3);
        clearValues[0].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[2].depthStencil = { .depth = 1.0f, .stencil = 0 };

        RF::BeginRenderPass(
            GetCommandBuffer(drawPass),
            mVkDisplayRenderPass,
            getDisplayFrameBuffer(drawPass),
            swapChainExtend,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalEndRenderPass(RT::DrawPass & drawPass)
    {
        if (RF::IsWindowVisible() == false)
        {
            return;
        }

        RF::EndRenderPass(GetCommandBuffer(drawPass));

        auto const presentQueueFamily = RF::GetPresentQueueFamily();
        auto const graphicQueueFamily = RF::GetGraphicQueueFamily();

        // If present and graphics queue families differ, then another barrier is required
        if (presentQueueFamily != graphicQueueFamily)
        {
            // TODO Check that WTF is this ?
            VkImageSubresourceRange const subResourceRange{
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
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalResize()
    {

        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        auto const swapChainExtent2D = VkExtent2D{
            .width = swapChainExtend.width,
            .height = swapChainExtend.height
        };

        // Depth image
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            RF::DestroyDepthImage(mDepthImageGroupList[i]);
            mDepthImageGroupList[i] = RF::CreateDepthImage(swapChainExtent2D, RT::CreateDepthImageOptions{
                .samplesCount = RF::GetMaxSamplesCount()
            });
        }

        // MSAA image
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            RF::DestroyColorImage(mMSAAImageGroupList[i]);
            mMSAAImageGroupList[i] = RF::CreateColorImage(
                swapChainExtent2D,
                mSwapChainImages.swapChainFormat,
                RT::CreateColorImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }

        // Swap-chain
        auto const oldSwapChainImages = mSwapChainImages;
        mSwapChainImages = RF::CreateSwapChain(oldSwapChainImages.swapChain);
        RF::DestroySwapChain(oldSwapChainImages);

        // Display frame-buffer
        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDisplayFrameBuffers.size()),
            mDisplayFrameBuffers.data()
        );
        createDisplayFrameBuffers(swapChainExtent2D);

        // Depth frame-buffer
        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDepthFrameBuffers.size()),
            mDepthFrameBuffers.data()
        );
        createDepthFrameBuffers(swapChainExtent2D);
    }

    //-------------------------------------------------------------------------------------------------

    VkSemaphore DisplayRenderPass::getImageAvailabilitySemaphore(RT::DrawPass const & drawPass)
    {
        return mSyncObjects.imageAvailabilitySemaphores[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkSemaphore DisplayRenderPass::getRenderFinishIndicatorSemaphore(RT::DrawPass const & drawPass)
    {
        return mSyncObjects.renderFinishIndicatorSemaphores[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkFence DisplayRenderPass::getInFlightFence(RT::DrawPass const & drawPass)
    {
        return mSyncObjects.fencesInFlight[drawPass.frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkImage DisplayRenderPass::getSwapChainImage(RT::DrawPass const & drawPass)
    {
        return mSwapChainImages.swapChainImages[drawPass.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderPass::getDisplayFrameBuffer(RT::DrawPass const & drawPass)
    {
        return mDisplayFrameBuffers[drawPass.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderPass::getDepthFrameBuffer(RT::DrawPass const & drawPass)
    {
        return mDepthFrameBuffers[drawPass.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::createDisplayFrameBuffers(VkExtent2D const & extent)
    {
        mDisplayFrameBuffers.clear();
        mDisplayFrameBuffers.resize(mSwapChainImagesCount);
        for (int i = 0; i < static_cast<int>(mDisplayFrameBuffers.size()); ++i)
        {
            std::vector<VkImageView> const attachments = {
                mMSAAImageGroupList[i].imageView,
                mSwapChainImages.swapChainImageViews[i],
                mDepthImageGroupList[i].imageView
            };
            mDisplayFrameBuffers[i] = RF::CreateFrameBuffer(
                mVkDisplayRenderPass,
                attachments.data(),
                static_cast<uint32_t>(attachments.size()),
                extent,
                1
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::createDepthFrameBuffers(VkExtent2D const & extent)
    {
        mDepthFrameBuffers.clear();
        mDepthFrameBuffers.resize(mSwapChainImagesCount);
        for (int i = 0; i < static_cast<int>(mDepthFrameBuffers.size()); ++i)
        {
            std::vector<VkImageView> const attachments = {
               mDepthImageGroupList[i].imageView
            };
            mDepthFrameBuffers[i] = RF::CreateFrameBuffer(
                mDepthRenderPass,
                attachments.data(),
                static_cast<uint32_t>(attachments.size()),
                extent,
                1
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::createDisplayRenderPass()
    {

        // Multisampled attachment that we render to
        VkAttachmentDescription const msaaAttachment{
            .format = mSwapChainImages.swapChainFormat,
            .samples = RF::GetMaxSamplesCount(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentDescription const swapChainAttachment{
            .format = mSwapChainImages.swapChainFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentDescription const depthAttachment{
            .format = mDepthImageGroupList[0].imageFormat,
            .samples = RF::GetMaxSamplesCount(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        // Note: hardware will automatically transition attachment to the specified layout
        // Note: index refers to attachment descriptions array
        VkAttachmentReference msaaAttachmentReference{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentReference swapChainAttachmentReference{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentReference depthAttachmentRef{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        // Note: this is a description of how the attachments of the render pass will be used in this sub pass
        // e.g. if they will be read in shaders and/or drawn to
        std::vector<VkSubpassDescription> subPassDescription{
            VkSubpassDescription {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &msaaAttachmentReference,
                .pResolveAttachments = &swapChainAttachmentReference,
                .pDepthStencilAttachment = &depthAttachmentRef,
            }
        };

        // Subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies{
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

        std::vector<VkAttachmentDescription> attachments = { msaaAttachment, swapChainAttachment, depthAttachment };

        mVkDisplayRenderPass = RF::CreateRenderPass(
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            subPassDescription.data(),
            static_cast<uint32_t>(subPassDescription.size()),
            dependencies.data(),
            static_cast<uint32_t>(dependencies.size())
        );
    }


    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::createDepthRenderPass()
    {

        VkAttachmentDescription const depthAttachment{
            .format = mDepthImageGroupList[0].imageFormat,
            .samples = RF::GetMaxSamplesCount(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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

        // TODO Fill dependencies correctly
        // Subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies{
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

        std::vector<VkAttachmentDescription> attachments{ depthAttachment };

        mDepthRenderPass = RF::CreateRenderPass(
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            subPassDescription.data(),
            static_cast<uint32_t>(subPassDescription.size()),
            dependencies.data(),
            static_cast<uint32_t>(dependencies.size())
        );

    }

    //-------------------------------------------------------------------------------------------------

}
