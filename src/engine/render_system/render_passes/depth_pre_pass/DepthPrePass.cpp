#include "DepthPrePass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"

//-------------------------------------------------------------------------------------------------

VkRenderPass MFA::DepthPrePass::GetVkRenderPass()
{
    return mRenderPass;
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::internalInit()
{
    auto * displayRenderPass = RF::GetDisplayRenderPass();
    MFA_ASSERT(displayRenderPass != nullptr);

    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtent = VkExtent2D {
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    createRenderPass();

    createFrameBuffers(swapChainExtent);
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::internalShutdown()
{
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()),
        mFrameBuffers.data()
    );

    RF::DestroyRenderPass(mRenderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::BeginRenderPass(RT::CommandRecordState & recordState)
{
    RenderPass::BeginRenderPass(recordState);
    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D{
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    RF::AssignViewportAndScissorToCommandBuffer(RF::GetGraphicCommandBuffer(recordState), swapChainExtend);

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(1);
    clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0 };

    RF::BeginRenderPass(
        RF::GetGraphicCommandBuffer(recordState),
        mRenderPass,
        getFrameBuffer(recordState),
        swapChainExtend,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::EndRenderPass(RT::CommandRecordState & recordState)
{
    RenderPass::EndRenderPass(recordState);
    RF::EndRenderPass(RF::GetGraphicCommandBuffer(recordState));
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::OnResize()
{
    auto surfaceCapabilities = RF::GetSurfaceCapabilities();
    auto const swapChainExtend = VkExtent2D{
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height
    };

    // Depth frame-buffer
    RF::DestroyFrameBuffers(
        static_cast<uint32_t>(mFrameBuffers.size()),
        mFrameBuffers.data()
    );
    createFrameBuffers(swapChainExtend);
}

//-------------------------------------------------------------------------------------------------

void MFA::DepthPrePass::createRenderPass()
{

    VkAttachmentDescription const depthAttachment{
        .format = RF::GetDisplayRenderPass()->GetDepthImages()[0]->imageFormat,
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

void MFA::DepthPrePass::createFrameBuffers(VkExtent2D const & extent)
{
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetSwapChainImagesCount());
    for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments = {RF::GetDisplayRenderPass()->GetDepthImages()[i]->imageView->imageView};
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

VkFramebuffer MFA::DepthPrePass::getFrameBuffer(RT::CommandRecordState const & drawPass)
{
    return mFrameBuffers[drawPass.imageIndex];
}

//-------------------------------------------------------------------------------------------------
