#include "OffScreenRenderPass.hpp"

namespace MFA {

VkRenderPass OffScreenRenderPass::GetVkRenderPass() {
    return mVkRenderPass;
}

VkCommandBuffer OffScreenRenderPass::GetCommandBuffer(RF::DrawPass const & drawPass) {
    return mCommandBuffer;
}

void OffScreenRenderPass::internalInit() {
    MFA_ASSERT(mImageWidth > 0);
    MFA_ASSERT(mImageHeight > 0);

    auto const shadowExtend = VkExtent2D {
        .width = mImageWidth,
        .height = mImageHeight
    };

    // TODO We might need to change usageFlags: VK_IMAGE_USAGE_TRANSFER_SRC_BIT feels extra
    mDepthImageGroup = RF::CreateDepthImage(shadowExtend, RB::CreateDepthImageOptions {
        .sliceCount = 6, // * light count
        .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    });

    createRenderPass();
    
    createFrameBuffer(shadowExtend);

    // TODO We might need special sampler as well
}

void OffScreenRenderPass::internalShutdown() {
    RF::DestroyFrameBuffers(1, &mFrameBuffer);
    RF::DestroyRenderPass(mVkRenderPass);
    RF::DestroyDepthImage(mDepthImageGroup);
}

void OffScreenRenderPass::internalBeginRenderPass(RF::DrawPass const & drawPass) {
    // TODO
}

void OffScreenRenderPass::internalEndRenderPass(RF::DrawPass const & drawPass) {
    // TODO
}

void OffScreenRenderPass::internalResize() {
}

void OffScreenRenderPass::createRenderPass() {
    std::vector<VkAttachmentDescription> attachments {};

    // Color attachment
    // This attachment is used for debugging, It is better that it exists but currently I have to handle too many things
    //attachments.emplace_back(VkAttachmentDescription {
    //    .format = SHADOW_MAP_FORMAT,
    //    .samples = VK_SAMPLE_COUNT_1_BIT,
    //    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    //    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    //    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    //    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    //    .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //    .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //});
    
    // Depth attachment
    attachments.emplace_back(VkAttachmentDescription {
        .format = mDepthImageGroup.imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    });
    
    //VkAttachmentReference colorReference {
    //    .attachment = 0,
    //    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //};

    VkAttachmentReference depthReference {
        .attachment = 0, // 1
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    
    std::vector<VkSubpassDescription> subPasses {
        VkSubpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,//&colorReference,
            .pDepthStencilAttachment = &depthReference,
        }
    };
    
    std::vector<VkSubpassDependency> dependencies {
        VkSubpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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

void OffScreenRenderPass::createFrameBuffer(VkExtent2D const & shadowExtent) {

    // Note: This comment is useful though does not match 100% with my code
    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    
    std::vector<VkImageView> const attachments = {mDepthImageGroup.imageView};
    mFrameBuffer = RF::CreateFrameBuffer(
        mVkRenderPass, 
        attachments.data(), 
        static_cast<uint32_t>(attachments.size()),
        shadowExtent,
        6   // * lightCount
    );
}

}
