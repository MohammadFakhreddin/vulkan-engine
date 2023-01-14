#pragma once

#include "engine/render_system/render_resources/RenderResource.hpp"
#include "engine/render_system/RenderTypes.hpp"

#include <memory>

namespace MFA
{
    class SwapChainRenderResource final : public RenderResource
    {
    public:

        explicit SwapChainRenderResource();

        ~SwapChainRenderResource() override;

        SwapChainRenderResource(SwapChainRenderResource const &) noexcept = delete;
        SwapChainRenderResource(SwapChainRenderResource &&) noexcept = delete;
        SwapChainRenderResource & operator = (SwapChainRenderResource const &) noexcept = delete;
        SwapChainRenderResource & operator = (SwapChainRenderResource &&) noexcept = delete;

        [[nodiscard]]
        VkImage GetSwapChainImage(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::SwapChainGroup const & GetSwapChainImages() const;

    protected:

        void OnResize() override;

    private:

        std::shared_ptr<RT::SwapChainGroup> mSwapChainImages{};

    };
}
