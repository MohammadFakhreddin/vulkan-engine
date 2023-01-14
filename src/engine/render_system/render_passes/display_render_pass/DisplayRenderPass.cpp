#include "DisplayRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderBackend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    DisplayRenderPass::DisplayRenderPass(
        std::shared_ptr<SwapChainRenderResource> swapChain,
        std::shared_ptr<DepthRenderResource> depth,
        std::shared_ptr<MSSAA_RenderResource> msaa
    )
        : RenderPass()
        , mSwapChain(std::move(swapChain))
        , mDepth(std::move(depth))
        , mMSAA(std::move(msaa))
    {
        CreateFrameBuffers();
    }

    //-------------------------------------------------------------------------------------------------

    DisplayRenderPass::~DisplayRenderPass() = default;

    //-------------------------------------------------------------------------------------------------

    VkRenderPass DisplayRenderPass::GetVkRenderPass()
    {
        return mVkRenderPass;
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::Init(std::shared_ptr<DisplayRenderResources> resources)
    {
        mResources = std::move(resources);



        CreateRenderPass();
        CreatePresentToDrawBarrier();
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::Shutdown()
    {
        mFrameBuffers.clear();
        RF::DestroyRenderPass(mVkRenderPass);
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::BeginRenderPass(RT::CommandRecordState & recordState)
    {
        ClearDepthBufferIfNeeded(recordState);

        RenderPass::BeginRenderPass(recordState);

        UsePresentToDrawBarrier(recordState);

        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        RF::AssignViewportAndScissorToCommandBuffer(recordState.commandBuffer, swapChainExtend);

        std::vector<VkClearValue> clearValues{};
        clearValues.resize(3);
        clearValues[0].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[1].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
        clearValues[2].depthStencil = { .depth = 1.0f, .stencil = 0 };

        RF::BeginRenderPass(
            recordState.commandBuffer,
            mVkRenderPass,
            GetFrameBuffer(recordState),
            swapChainExtend,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::EndRenderPass(RT::CommandRecordState & recordState)
    {
        RenderPass::EndRenderPass(recordState);

        if (RF::IsWindowVisible() == false)
        {
            return;
        }

        RF::EndRenderPass(recordState.commandBuffer);

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
            drawToPresentBarrier.image = mResources->GetSwapChainImage(recordState);
            drawToPresentBarrier.subresourceRange = subResourceRange;

            RF::PipelineBarrier(
                recordState,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                1,
                &drawToPresentBarrier
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::NotifyDepthImageLayoutIsSet()
    {
        mIsDepthImageUndefined = false;
    }

    //-------------------------------------------------------------------------------------------------

    // Static approach for initial layout of depth buffers
    /*void DisplayRenderPass::UseDepthImageLayoutAsUndefined(bool const setDepthImageLayoutAsUndefined)
    {
        if (mIsDepthImageInitialLayoutUndefined == setDepthImageLayoutAsUndefined)
        {
            return;
        }
        mIsDepthImageInitialLayoutUndefined = setDepthImageLayoutAsUndefined;

        RF::DeviceWaitIdle();

        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        RF::DestroyRenderPass(mVkDisplayRenderPass);

        createDisplayRenderPass();

        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mDisplayFrameBuffers.size()),
            mDisplayFrameBuffers.data()
        );

        createDisplayFrameBuffers(swapChainExtend);
    }*/



    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderPass::GetFrameBuffer(RT::CommandRecordState const & recordState) const
    {
        return mFrameBuffers[recordState.imageIndex]->framebuffer;
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderPass::GetFrameBuffer(uint32_t const imageIndex) const
    {
        return mFrameBuffers[imageIndex]->framebuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::CreateFrameBuffers()
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtent = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        mFrameBuffers.clear();
        mFrameBuffers.resize(RF::GetSwapChainImagesCount());
        for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
        {
            std::vector<VkImageView> const attachments{
                mMSAA->GetImageGroup(i).imageView->imageView,
                mSwapChain->GetSwapChainImages().swapChainImageViews[i]->imageView,
                mDepth->GetDepthImage(i).imageView->imageView
            };
            mFrameBuffers[i] = std::make_shared<RT::FrameBuffer>(RF::CreateFrameBuffer(
                mVkRenderPass,
                attachments.data(),
                static_cast<uint32_t>(attachments.size()),
                swapChainExtent,
                1
            ));
        }
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::CreateRenderPass()
    {
        auto format = RF::GetSurfaceFormat().format;
        // Multi-sampled attachment that we render to
        VkAttachmentDescription const msaaAttachment{
            .format = format,
            .samples = RF::GetMaxSamplesCount(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentDescription const swapChainAttachment{
            .format = RF::GetSurfaceFormat().format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentDescription const depthAttachment{
            .format = RF::GetDepthFormat(),
            .samples = RF::GetMaxSamplesCount(),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
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

        std::vector<VkAttachmentDescription> attachments = { msaaAttachment, swapChainAttachment, depthAttachment };

        mVkRenderPass = RF::CreateRenderPass(
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            subPassDescription.data(),
            static_cast<uint32_t>(subPassDescription.size()),
            nullptr,
            0
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::CreatePresentToDrawBarrier()
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

        VkImageSubresourceRange const subResourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        presentToDrawBarrier.subresourceRange = subResourceRange;

        mPresentToDrawBarrier = presentToDrawBarrier;
    }

    //-------------------------------------------------------------------------------------------------

    static void changeDepthImageLayout(
        VkCommandBuffer commandBuffer,
        RT::DepthImageGroup const & depthImage,
        VkImageLayout initialLayout,
        VkImageLayout finalLayout,
        VkImageSubresourceRange const & subResourceRange
    )
    {
        auto const imageBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = initialLayout,
            .newLayout = finalLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = depthImage.imageGroup->image,
            .subresourceRange = subResourceRange
        };

        std::vector<VkImageMemoryBarrier> const barriers {imageBarrier};
        RB::PipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            static_cast<uint32_t>(barriers.size()),
            barriers.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::ClearDepthBufferIfNeeded(RT::CommandRecordState const & recordState)
    {
        if (mIsDepthImageUndefined)
        {
            auto const & depthImage = mDepthImageGroupList[recordState.imageIndex];

            VkImageSubresourceRange const subResourceRange{
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            changeDepthImageLayout(
                recordState.commandBuffer,
                *depthImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL,
                subResourceRange
            );

            VkClearDepthStencilValue const depthStencil = { .depth = 1.0f, .stencil = 0 };

            // TODO Move to RF or RB
            vkCmdClearDepthStencilImage(
                recordState.commandBuffer,
                depthImage->imageGroup->image,
                VK_IMAGE_LAYOUT_GENERAL,
                &depthStencil,
                1,
                &subResourceRange
            );

            changeDepthImageLayout(
                recordState.commandBuffer,
                *depthImage,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                subResourceRange
            );
        }

        mIsDepthImageUndefined = true;
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderPass::UsePresentToDrawBarrier(RT::CommandRecordState const & recordState)
    {
        mPresentToDrawBarrier.image = mSwapChainImages->swapChainImages[recordState.imageIndex];

        std::vector<VkImageMemoryBarrier> const barriers {mPresentToDrawBarrier};

        RF::PipelineBarrier(
            recordState,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            static_cast<uint32_t>(barriers.size()),
            barriers.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

}
