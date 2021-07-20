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

protected:

    void internalInit() override;

    void internalShutdown() override;

    RF::DrawPass internalBegin() override;

    void internalEnd(RF::DrawPass & drawPass) override;

    void internalResize() override;

private:

    VkRenderPass mVkRenderPass {};
    RB::SwapChainGroup mSwapChainImages {};
    std::vector<VkFramebuffer> mFrameBuffers {};
    RB::DepthImageGroup mDepthImageGroup {};
    RB::SyncObjects mSyncObjects {};
    uint8_t mCurrentFrame = 0;
};

}
