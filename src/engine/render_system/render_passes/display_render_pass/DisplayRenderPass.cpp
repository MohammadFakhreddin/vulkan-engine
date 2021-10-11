#include "DisplayRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    VkRenderPass DisplayRenderPass::GetVkRenderPass()
    {
        return mVkDisplayRenderPass;
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

        createDisplayFrameBuffers(swapChainExtent);

    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalShutdown()
    {

        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDisplayFrameBuffers.size()),
            mDisplayFrameBuffers.data()
        );

        RF::DestroyRenderPass(mVkDisplayRenderPass);

        
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

    void DisplayRenderPass::internalBeginRenderPass(RT::CommandRecordState & drawPass)
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
            RF::GetGraphicCommandBuffer(drawPass),
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentToDrawBarrier
        );

        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        RF::AssignViewportAndScissorToCommandBuffer(RF::GetGraphicCommandBuffer(drawPass), swapChainExtend);

        std::vector<VkClearValue> clearValues{};
        clearValues.resize(3);
        clearValues[0].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[2].depthStencil = { .depth = 1.0f, .stencil = 0 };

        RF::BeginRenderPass(
            RF::GetGraphicCommandBuffer(drawPass),
            mVkDisplayRenderPass,
            getDisplayFrameBuffer(drawPass),
            swapChainExtend,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::internalEndRenderPass(RT::CommandRecordState & drawPass)
    {
        if (RF::IsWindowVisible() == false)
        {
            return;
        }

        RF::EndRenderPass(RF::GetGraphicCommandBuffer(drawPass));

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
            drawToPresentBarrier.image = GetSwapChainImage(drawPass);
            drawToPresentBarrier.subresourceRange = subResourceRange;

            RF::PipelineBarrier(
                RF::GetGraphicCommandBuffer(drawPass),
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

    }

    //-------------------------------------------------------------------------------------------------

    VkImage DisplayRenderPass::GetSwapChainImage(RT::CommandRecordState const & drawPass)
    {
        return mSwapChainImages.swapChainImages[drawPass.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    RT::SwapChainGroup const & DisplayRenderPass::GetSwapChainImages() const
    {
        return mSwapChainImages;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<RT::DepthImageGroup> const & DisplayRenderPass::GetDepthImages() const
    {
        return mDepthImageGroupList;
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderPass::getDisplayFrameBuffer(RT::CommandRecordState const & drawPass)
    {
        return mDisplayFrameBuffers[drawPass.imageIndex];
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

}
