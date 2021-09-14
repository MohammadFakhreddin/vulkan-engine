#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

class DisplayRenderPass final : public RenderPass {
public:

    DisplayRenderPass() = default;
    ~DisplayRenderPass() override = default;

    DisplayRenderPass (DisplayRenderPass const &) noexcept = delete;
    DisplayRenderPass (DisplayRenderPass &&) noexcept = delete;
    DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
    DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    VkCommandBuffer GetCommandBuffer(RT::DrawPass const & drawPass);

    [[nodiscard]]
    RT::DrawPass StartGraphicCommandBufferRecording();

    void EndGraphicCommandBufferRecording(RT::DrawPass & drawPass);

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RT::DrawPass & drawPass) override;

    void internalEndRenderPass(RT::DrawPass & drawPass) override;

    void internalResize() override;

private:

    VkSemaphore getImageAvailabilitySemaphore(RT::DrawPass const & drawPass);

    VkSemaphore getRenderFinishIndicatorSemaphore(RT::DrawPass const & drawPass);

    VkFence getInFlightFence(RT::DrawPass const & drawPass);

    VkImage getSwapChainImage(RT::DrawPass const & drawPass);

    VkFramebuffer getFrameBuffer(RT::DrawPass const & drawPass);

    void createFrameBuffers(VkExtent2D const & extent);

    void createRenderPass();
    
    VkRenderPass mVkRenderPass {};
    uint32_t mSwapChainImagesCount = 0;
    RT::SwapChainGroup mSwapChainImages {};
    std::vector<VkFramebuffer> mFrameBuffers {};
    RT::ColorImageGroup mMSAAImageGroup {};
    RT::DepthImageGroup mDepthImageGroup {};
    RT::SyncObjects mSyncObjects {};
    uint8_t mCurrentFrame = 0;
    std::vector<VkCommandBuffer> mGraphicCommandBuffers {};

};

}
