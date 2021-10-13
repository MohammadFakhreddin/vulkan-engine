#include "OcclusionRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::OcclusionRenderPass::OcclusionRenderPass()
{
}

//-------------------------------------------------------------------------------------------------

VkRenderPass MFA::OcclusionRenderPass::GetVkRenderPass()
{
    return mRenderPass;
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalInit()
{
    auto const capabilities = RF::GetSurfaceCapabilities();
    mImageWidth = static_cast<uint32_t>(static_cast<float>(capabilities.currentExtent.width) / 4.0f);
    mImageHeight = static_cast<uint32_t>(static_cast<float>(capabilities.currentExtent.height) / 4.0f);

    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    // Color images
    mColorImages.resize(RF::GetSwapChainImagesCount());
    for (auto & colorImage : mColorImages)
    {
        colorImage = RF::CreateColorImage(
            extent,
            VK_FORMAT_R8_UINT,
            {}
        );
    }

    // Depth images
    mDepthImages.resize(RF::GetSwapChainImagesCount());
    for (auto & depthImage : mDepthImages)
    {
        depthImage = RF::CreateDepthImage(extent, {});
    }

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

    // Color images
    for (auto & colorImage : mColorImages)
    {
        RF::DestroyColorImage(colorImage);
    }
    mColorImages.clear();

    // Depth images
    for (auto & depthImage : mDepthImages)
    {
        RF::DestroyDepthImage(depthImage);
    }
    mDepthImages.clear();
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalBeginRenderPass(RT::CommandRecordState & drawPass)
{
    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    RF::AssignViewportAndScissorToCommandBuffer(RF::GetGraphicCommandBuffer(drawPass), extent);

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = VkClearColorValue{ .float32 = {0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { .depth = 1.0f, .stencil = 0 };

    RF::BeginRenderPass(
        RF::GetGraphicCommandBuffer(drawPass),
        mRenderPass,
        getFrameBuffer(drawPass),
        extent,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalEndRenderPass(RT::CommandRecordState & drawPass)
{
    RF::EndRenderPass(RF::GetGraphicCommandBuffer(drawPass));
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::internalResize()
{
    auto const capabilities = RF::GetSurfaceCapabilities();
    mImageWidth = static_cast<uint32_t>(static_cast<float>(capabilities.currentExtent.width) / 4.0f);
    mImageHeight = static_cast<uint32_t>(static_cast<float>(capabilities.currentExtent.height) / 4.0f);

    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    for (auto & colorImage : mColorImages)
    {
        RF::DestroyColorImage(colorImage);
        colorImage = RF::CreateColorImage(extent, colorImage.imageFormat, {});
    }

    for (auto & depthImage : mDepthImages)
    {
        RF::DestroyDepthImage(depthImage);
        depthImage = RF::CreateDepthImage(extent, {});
    }

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
    VkAttachmentDescription const colorAttachment{
        .format = VK_FORMAT_R8_UINT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorAttachmentReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription const depthAttachment{
        .format = mDepthImages[0].imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };


    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    std::vector<VkSubpassDescription> subPassDescription{
        VkSubpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentReference,
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

    std::vector<VkAttachmentDescription> attachments{ colorAttachment, depthAttachment };

    mRenderPass = RF::CreateRenderPass(
        attachments.data(),
        static_cast<uint32_t>(attachments.size()),
        subPassDescription.data(),
        static_cast<uint32_t>(subPassDescription.size()),
        dependencies.data(),
        static_cast<uint32_t>(dependencies.size())
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::OcclusionRenderPass::createFrameBuffers(VkExtent2D const & extent)
{
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetSwapChainImagesCount());
    for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments = {
            mColorImages[i].imageView,
            mDepthImages[i].imageView
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

VkFramebuffer MFA::OcclusionRenderPass::getFrameBuffer(RT::CommandRecordState const & drawPass)
{
    return mFrameBuffers[drawPass.imageIndex];
}

//-------------------------------------------------------------------------------------------------
