#include "OcclusionRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"

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
    RenderPass::BeginRenderPass(recordState);

    auto const extent = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

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
        .format = RF::GetDisplayRenderPass()->GetDepthImages()[0]->imageFormat,
        .samples = RF::GetMaxSamplesCount(),
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
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
    auto & depthImages = RF::GetDisplayRenderPass()->GetDepthImages();
    for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments {depthImages[i]->imageView->imageView};
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
