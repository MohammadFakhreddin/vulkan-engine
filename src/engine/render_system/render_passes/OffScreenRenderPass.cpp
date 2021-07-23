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

    mDepthImageGroup = RF::CreateDepthImage(shadowExtend, RB::CreateDepthImageOptions {
        .sliceCount = 6, // * light count
        .usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    });

    //mVkRenderPass = RF::CreateRenderPass(mSwapChainImages.swapChainFormat);

    //mFrameBuffer = RF::CreateFrameBuffer()
}

void OffScreenRenderPass::internalShutdown() {
    // TODO
}

void OffScreenRenderPass::internalBeginRenderPass(RF::DrawPass const & drawPass) {
    // TODO
}

void OffScreenRenderPass::internalEndRenderPass(RF::DrawPass const & drawPass) {
    // TODO
}

void OffScreenRenderPass::internalResize() {
    // TODO
}

}