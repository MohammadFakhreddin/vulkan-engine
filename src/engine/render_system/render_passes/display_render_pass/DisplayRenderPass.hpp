#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/render_resources/swapchain/SwapChainRenderResource.hpp"
#include "engine/render_system/render_resources/depth/DepthRenderResource.hpp"
#include "engine/render_system/render_resources/msaa/MSAA_RenderResource.hpp"

namespace MFA
{

    class DisplayRenderPass final : public RenderPass
    {
    public:

        explicit DisplayRenderPass(
            std::shared_ptr<SwapChainRenderResource> swapChain,
            std::shared_ptr<DepthRenderResource> depth,
            std::shared_ptr<MSSAA_RenderResource> msaa
        );

        ~DisplayRenderPass() override;

        DisplayRenderPass(DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass(DisplayRenderPass &&) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;
        
        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState);
        
        void NotifyDepthImageLayoutIsSet();
        
    private:


        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t imageIndex) const;

        void CreateFrameBuffers();

        void CreateRenderPass();
        
        void CreatePresentToDrawBarrier();

        void ClearDepthBufferIfNeeded(RT::CommandRecordState const & recordState);
        
        void UsePresentToDrawBarrier(RT::CommandRecordState const & recordState);

        void OnResize() override;

        VkRenderPass mVkRenderPass{};            // TODO Make this a renderType
        
        VkImageMemoryBarrier mPresentToDrawBarrier {};

        bool mIsDepthImageUndefined = true;

        std::shared_ptr<SwapChainRenderResource> mSwapChain{};
        std::shared_ptr<DepthRenderResource> mDepth {};
        std::shared_ptr<MSSAA_RenderResource> mMSAA {};

        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};

    };

}
