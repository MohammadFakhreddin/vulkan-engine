#include "DisplayRenderResources.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderBackend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    DisplayRenderResources::DisplayRenderResources()
        : RenderResource()
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtent = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        mSwapChainImagesCount = RF::GetSwapChainImagesCount();

        mSwapChainImages = RF::CreateSwapChain();

        mMSAAImageGroupList.resize(mSwapChainImagesCount);
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            mMSAAImageGroupList[i] = RF::CreateColorImage(
                swapChainExtent,
                mSwapChainImages->swapChainFormat,
                RT::CreateColorImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }

        CreateDepthImages(swapChainExtent);
    }

    //-------------------------------------------------------------------------------------------------

    DisplayRenderResources::~DisplayRenderResources()
    {
        mSwapChainImages.reset();
        mMSAAImageGroupList.clear();
        mDepthImageGroupList.clear();
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderResources::CreateDepthImages(VkExtent2D const & extent2D)
    {
        mDepthImageGroupList.resize(mSwapChainImagesCount);
        for (auto & depthImage : mDepthImageGroupList)
        {
            depthImage = RF::CreateDepthImage(
                extent2D,
                RT::CreateDepthImageOptions{
                    .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }
    }


    //-------------------------------------------------------------------------------------------------

    VkImage DisplayRenderResources::GetSwapChainImage(RT::CommandRecordState const & recordState) const
    {
        return mSwapChainImages->swapChainImages[recordState.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    RT::SwapChainGroup const & DisplayRenderResources::GetSwapChainImages() const
    {
        return *mSwapChainImages;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<std::shared_ptr<RT::DepthImageGroup>> const & DisplayRenderResources::GetDepthImages() const
    {
        return mDepthImageGroupList;
    }
    //-------------------------------------------------------------------------------------------------

    void DisplayRenderResources::OnResize()
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        // Depth image
        CreateDepthImages(swapChainExtend);

        // MSAA image
        for (uint32_t i = 0; i < mSwapChainImagesCount; ++i)
        {
            mMSAAImageGroupList[i] = RF::CreateColorImage(
                swapChainExtend,
                mSwapChainImages->swapChainFormat,
                RT::CreateColorImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }

        // Swap-chain
        auto const oldSwapChainImages = mSwapChainImages;
        mSwapChainImages = RF::CreateSwapChain(oldSwapChainImages->swapChain);

        // Display frame-buffer
        RF::DestroyFrameBuffers(
            static_cast<uint32_t>(mFrameBuffers.size()),
            mFrameBuffers.data()
        );
        CreateDisplayFrameBuffers(swapChainExtend);
    }

    //-------------------------------------------------------------------------------------------------

}
