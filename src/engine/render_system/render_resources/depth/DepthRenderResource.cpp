#include "DepthRenderResource.hpp"

#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    DepthRenderResource::DepthRenderResource()
        : RenderResource()
    {
        CreateDepthImages();
    }

    //-------------------------------------------------------------------------------------------------

    DepthRenderResource::~DepthRenderResource()
    {
        mDepthImageGroupList.clear();
    }

    //-------------------------------------------------------------------------------------------------

    RT::DepthImageGroup const& DepthRenderResource::GetDepthImage(int const index) const
    {
        return *mDepthImageGroupList[index];
    }

    //-------------------------------------------------------------------------------------------------

    void DepthRenderResource::OnResize()
    {
        mDepthImageGroupList.clear();
        CreateDepthImages();
    }

    //-------------------------------------------------------------------------------------------------

    void DepthRenderResource::CreateDepthImages()
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        mDepthImageGroupList.resize(RF::GetSwapChainImagesCount());
        for (auto & depthImage : mDepthImageGroupList)
        {
            depthImage = RF::CreateDepthImage(
                swapChainExtend,
                RT::CreateDepthImageOptions{
                    .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
