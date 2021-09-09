#pragma once

#include "RenderPass.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;
namespace RB = RenderBackend;

class DisplayRenderPass final : public RenderPass {
public:

    DisplayRenderPass() = default;
    ~DisplayRenderPass() override = default;

    DisplayRenderPass (DisplayRenderPass const &) noexcept = delete;
    DisplayRenderPass (DisplayRenderPass &&) noexcept = delete;
    DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
    DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    VkCommandBuffer GetCommandBuffer(RF::DrawPass const & drawPass);

    [[nodiscard]]
    RF::DrawPass StartGraphicCommandBufferRecording();

    void EndGraphicCommandBufferRecording(RF::DrawPass & drawPass);

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RF::DrawPass const & drawPass) override;

    void internalEndRenderPass(RF::DrawPass const & drawPass) override;

    void internalResize() override;

private:

    VkSemaphore getImageAvailabilitySemaphore(RF::DrawPass const & drawPass);

    VkSemaphore getRenderFinishIndicatorSemaphore(RF::DrawPass const & drawPass);

    VkFence getInFlightFence(RF::DrawPass const & drawPass);

    VkImage getSwapChainImage(RF::DrawPass const & drawPass);

    VkFramebuffer getFrameBuffer(RF::DrawPass const & drawPass);

    void createFrameBuffers(VkExtent2D const & extent);

    void createRenderPass();
    
    VkRenderPass mVkRenderPass {};
    uint32_t mSwapChainImagesCount = 0;
    RB::SwapChainGroup mSwapChainImages {};
    std::vector<VkFramebuffer> mFrameBuffers {};
    RB::ColorImageGroup mMSAAImageGroup {};
    RB::DepthImageGroup mDepthImageGroup {};
    RB::SyncObjects mSyncObjects {};
    uint8_t mCurrentFrame = 0;
    std::vector<VkCommandBuffer> mGraphicCommandBuffers {};

};

}
