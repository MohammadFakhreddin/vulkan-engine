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

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;

        [[nodiscard]]
        VkImage GetSwapChainImage(RT::CommandRecordState const & drawPass) const;

        [[nodiscard]]
        RT::SwapChainGroup const & GetSwapChainImages() const;

        [[nodiscard]]
        std::vector<std::shared_ptr<RT::DepthImageGroup>> const & GetDepthImages() const;

        void BeginRenderPass(RT::CommandRecordState & recordState) override;

        void EndRenderPass(RT::CommandRecordState & recordState) override;

        void OnResize() override;

        // TODO Find a better way
        //void UseDepthImageLayoutAsUndefined(bool setDepthImageLayoutAsUndefined);

        void notifyDepthImageLayoutIsSet();

    protected:

        void internalInit() override;

        void internalShutdown() override;

    private:

        [[nodiscard]]
        VkFramebuffer getDisplayFrameBuffer(RT::CommandRecordState const & drawPass) const;

        void createDisplayFrameBuffers(VkExtent2D const & extent);

        void createDisplayRenderPass();

        void createDepthImages(VkExtent2D const & extent2D);

        void createPresentToDrawBarrier();

        void clearDepthBufferIfNeeded(RT::CommandRecordState const & recordState);
        
        void usePresentToDrawBarrier(RT::CommandRecordState const & recordState);

        VkRenderPass mVkDisplayRenderPass{};            // TODO Make this a renderType
        uint32_t mSwapChainImagesCount = 0;
        std::shared_ptr<RT::SwapChainGroup> mSwapChainImages{};
        std::vector<VkFramebuffer> mDisplayFrameBuffers{};
        std::vector<std::shared_ptr<RT::ColorImageGroup>> mMSAAImageGroupList{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList{};

        VkImageMemoryBarrier mPresentToDrawBarrier {};

        bool mIsDepthImageUndefined = true;
    };

}
