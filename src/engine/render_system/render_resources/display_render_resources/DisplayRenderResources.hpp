#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_resources/RenderResource.hpp"

namespace MFA
{
    class DisplayRenderResources : RenderResource
    {
    public:

        explicit DisplayRenderResources();
        ~DisplayRenderResources();

        DisplayRenderResources(DisplayRenderResources const &) noexcept = delete;
        DisplayRenderResources(DisplayRenderResources &&) noexcept = delete;
        DisplayRenderResources & operator = (DisplayRenderResources const &) noexcept = delete;
        DisplayRenderResources & operator = (DisplayRenderResources &&) noexcept = delete;

        void OnResize() override;

        [[nodiscard]]
        VkImage GetSwapChainImage(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::SwapChainGroup const & GetSwapChainImages() const;

        [[nodiscard]]
        std::vector<std::shared_ptr<RT::DepthImageGroup>> const & GetDepthImages() const;

    private:

        void CreateDepthImages(VkExtent2D const & extent2D);

        uint32_t mSwapChainImagesCount = 0;
        std::shared_ptr<RT::SwapChainGroup> mSwapChainImages{};
        std::vector<std::shared_ptr<RT::ColorImageGroup>> mMSAAImageGroupList{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList{};

    };
}
