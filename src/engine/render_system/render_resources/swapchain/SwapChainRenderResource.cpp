#include "SwapChainRenderResource.hpp"

#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    SwapChainRenderResource::SwapChainRenderResource()
    {
        mSwapChainImages = RF::CreateSwapChain();
    }

    //-------------------------------------------------------------------------------------------------

    SwapChainRenderResource::~SwapChainRenderResource()
    {
        mSwapChainImages.reset();
    }

    //-------------------------------------------------------------------------------------------------

    VkImage SwapChainRenderResource::GetSwapChainImage(RT::CommandRecordState const& recordState) const
    {
        return mSwapChainImages->swapChainImages[recordState.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    RT::SwapChainGroup const& SwapChainRenderResource::GetSwapChainImages() const
    {
        return *mSwapChainImages;
    }

    //-------------------------------------------------------------------------------------------------

    void SwapChainRenderResource::OnResize()
    {
        // Swap-chain
        auto const oldSwapChainImages = mSwapChainImages;
        mSwapChainImages = RF::CreateSwapChain(oldSwapChainImages->swapChain);
    }

    //-------------------------------------------------------------------------------------------------

}
