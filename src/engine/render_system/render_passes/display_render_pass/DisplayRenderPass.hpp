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

    [[nodiscard]]
    VkRenderPass GetVkRenderPass() override;

    [[nodiscard]]
    VkRenderPass GetVkDepthRenderPass() const;

    VkCommandBuffer GetCommandBuffer(RT::DrawPass const & drawPass);

    [[nodiscard]]
    RT::DrawPass StartGraphicCommandBufferRecording();

    void EndGraphicCommandBufferRecording(RT::DrawPass & drawPass);

    void BeginDepthPrePass(RT::DrawPass & drawPass);

    void EndDepthPrePass(RT::DrawPass & drawPass);

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RT::DrawPass & drawPass) override;

    void internalEndRenderPass(RT::DrawPass & drawPass) override;

    void internalResize() override;

private:

    [[nodiscard]]
    VkSemaphore getImageAvailabilitySemaphore(RT::DrawPass const & drawPass);

    [[nodiscard]]
    VkSemaphore getRenderFinishIndicatorSemaphore(RT::DrawPass const & drawPass);

    [[nodiscard]]
    VkFence getInFlightFence(RT::DrawPass const & drawPass);

    [[nodiscard]]
    VkImage getSwapChainImage(RT::DrawPass const & drawPass);

    [[nodiscard]]
    VkFramebuffer getDisplayFrameBuffer(RT::DrawPass const & drawPass);

    [[nodiscard]]
    VkFramebuffer getDepthFrameBuffer(RT::DrawPass const & drawPass);

    void createDisplayFrameBuffers(VkExtent2D const & extent);

    void createDepthFrameBuffers(VkExtent2D const & extent);

    void createDisplayRenderPass();

    void createDepthRenderPass();
    
    VkRenderPass mVkDisplayRenderPass {};
    uint32_t mSwapChainImagesCount = 0;
    RT::SwapChainGroup mSwapChainImages {};
    std::vector<VkFramebuffer> mDisplayFrameBuffers {};
    std::vector<RT::ColorImageGroup> mMSAAImageGroupList {};
    std::vector<RT::DepthImageGroup> mDepthImageGroupList {};
    RT::SyncObjects mSyncObjects {};
    uint8_t mCurrentFrame = 0;
    std::vector<VkCommandBuffer> mGraphicCommandBuffers {};

    VkRenderPass mDepthRenderPass {};
    std::vector<VkFramebuffer> mDepthFrameBuffer {};

    RT::DrawPass mDrawPass {};
};

}
