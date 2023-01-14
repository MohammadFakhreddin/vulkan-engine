#include "MSAA_RenderResource.hpp"

#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    MSSAA_RenderResource::MSSAA_RenderResource()
        : RenderResource()
    {
        CreateMSAAImage();
    }

    //-------------------------------------------------------------------------------------------------

    MSSAA_RenderResource::~MSSAA_RenderResource()
    {
        mMSAAImageGroupList.clear();
    }

    //-------------------------------------------------------------------------------------------------

    RT::ColorImageGroup const& MSSAA_RenderResource::GetImageGroup(int const index) const
    {
        return *mMSAAImageGroupList[index];
    }

    //-------------------------------------------------------------------------------------------------

    void MSSAA_RenderResource::OnResize()
    {
        mMSAAImageGroupList.clear();
        CreateMSAAImage();
    }

    //-------------------------------------------------------------------------------------------------

    void MSSAA_RenderResource::CreateMSAAImage()
    {
        auto surfaceCapabilities = RF::GetSurfaceCapabilities();
        auto const swapChainExtend = VkExtent2D{
            .width = surfaceCapabilities.currentExtent.width,
            .height = surfaceCapabilities.currentExtent.height
        };

        mMSAAImageGroupList.resize(RF::GetSwapChainImagesCount());
        for (auto & MSAA_Image : mMSAAImageGroupList)
        {
            MSAA_Image = RF::CreateColorImage(
                swapChainExtend,
                RF::GetSurfaceFormat().format,
                RT::CreateColorImageOptions{
                    .samplesCount = RF::GetMaxSamplesCount()
                }
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
