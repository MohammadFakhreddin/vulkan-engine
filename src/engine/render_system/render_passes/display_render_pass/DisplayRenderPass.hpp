#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class DisplayRenderPass final : public RenderPass
    {
    public:

        explicit DisplayRenderPass();
        ~DisplayRenderPass() override;

        DisplayRenderPass(DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass(DisplayRenderPass &&) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

        void Init();

        void Shutdown();

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;

        [[nodiscard]]
        VkImage GetSwapChainImage(RT::CommandRecordState const & drawPass) const;

        [[nodiscard]]
        RT::SwapChainGroup const & GetSwapChainImages() const;

        [[nodiscard]]
        std::vector<std::shared_ptr<RT::DepthImageGroup>> const & GetDepthImages() const;

        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState);

        
        void notifyDepthImageLayoutIsSet();
        
    private:

        [[nodiscard]]
        VkFramebuffer getDisplayFrameBuffer(RT::CommandRecordState const & drawPass) const;

        void CreateDisplayFrameBuffers(VkExtent2D const & extent);

        void createDisplayRenderPass();
        
        void createPresentToDrawBarrier();

        void clearDepthBufferIfNeeded(RT::CommandRecordState const & recordState);
        
        void usePresentToDrawBarrier(RT::CommandRecordState const & recordState);

        VkRenderPass mVkDisplayRenderPass{};            // TODO Make this a renderType
        
        VkImageMemoryBarrier mPresentToDrawBarrier {};

        bool mIsDepthImageUndefined = true;
    };

}
