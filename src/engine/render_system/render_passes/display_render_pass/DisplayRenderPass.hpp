#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class DisplayRenderPass final : public RenderPass
    {
    public:

        DisplayRenderPass() = default;
        ~DisplayRenderPass() override = default;

        DisplayRenderPass(DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass(DisplayRenderPass &&) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;

        [[nodiscard]]
        VkImage GetSwapChainImage(RT::CommandRecordState const & drawPass);

        [[nodiscard]]
        RT::SwapChainGroup const & GetSwapChainImages() const;

        [[nodiscard]]
        std::vector<std::shared_ptr<RT::DepthImageGroup>> const & GetDepthImages() const;

        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState);

        void OnResize() override;

    protected:

        void internalInit() override;

        void internalShutdown() override;

    private:

        [[nodiscard]]
        VkFramebuffer getDisplayFrameBuffer(RT::CommandRecordState const & drawPass);

        void createDisplayFrameBuffers(VkExtent2D const & extent);

        void createDisplayRenderPass();

        VkRenderPass mVkDisplayRenderPass{};
        uint32_t mSwapChainImagesCount = 0;
        std::shared_ptr<RT::SwapChainGroup> mSwapChainImages{};
        std::vector<VkFramebuffer> mDisplayFrameBuffers{};
        std::vector<std::shared_ptr<RT::ColorImageGroup>> mMSAAImageGroupList{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList{};
    
    };

}
