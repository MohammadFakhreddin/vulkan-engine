#include "DisplayRenderResources.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderBackend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderResources::Init(VkRenderPass renderPass)
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

        createDepthImages(swapChainExtent);

        createDisplayRenderPass();

        createDisplayFrameBuffers(swapChainExtent);
    }

    //-------------------------------------------------------------------------------------------------

    void DisplayRenderResources::Shutdown()
    {
        mSwapChainImages.reset();
        mMSAAImageGroupList.clear();
        mDepthImageGroupList.clear();
        mFrameBuffers.clear();
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

    VkFramebuffer DisplayRenderResources::GetFrameBuffer(RT::CommandRecordState const & drawPass) const
    {
        return mFrameBuffers[drawPass.imageIndex];
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DisplayRenderResources::GetFrameBuffer(uint32_t const imageIndex) const
    {
        return mFrameBuffers[imageIndex];
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

    void DisplayRenderResources::CreateDisplayFrameBuffers(VkRenderPass renderPass, VkExtent2D const & extent)
    {
        mFrameBuffers.clear();
        mFrameBuffers.resize(mSwapChainImagesCount);
        for (int i = 0; i < static_cast<int>(mFrameBuffers.size()); ++i)
        {
            std::vector<VkImageView> const attachments{
                mMSAAImageGroupList[i]->imageView->imageView,
                mSwapChainImages->swapChainImageViews[i]->imageView,
                mDepthImageGroupList[i]->imageView->imageView
            };
            mFrameBuffers[i] = RF::CreateFrameBuffer(
                renderPass,
                attachments.data(),
                static_cast<uint32_t>(attachments.size()),
                extent,
                1
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
