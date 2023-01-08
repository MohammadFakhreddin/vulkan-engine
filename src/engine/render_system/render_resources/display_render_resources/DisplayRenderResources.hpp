#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_resources/RenderResources.hpp"

namespace MFA
{
    class DisplayRenderResources : RenderResources
    {
    public:

        void Init(VkRenderPass renderPass);

        void Shutdown();

        DisplayRenderResources(DisplayRenderResources const &) noexcept = delete;
        DisplayRenderResources(DisplayRenderResources &&) noexcept = delete;
        DisplayRenderResources & operator = (DisplayRenderResources const &) noexcept = delete;
        DisplayRenderResources & operator = (DisplayRenderResources &&) noexcept = delete;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & drawPass) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t imageIndex) const;

        void OnResize() override;

    private:

        void CreateDepthImages(VkExtent2D const & extent2D);

        uint32_t mSwapChainImagesCount = 0;
        std::shared_ptr<RT::SwapChainGroup> mSwapChainImages{};
        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};
        std::vector<std::shared_ptr<RT::ColorImageGroup>> mMSAAImageGroupList{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList{};

    };
}
