#include "PointLightShadowResource.hpp"

#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::PointLightShadowResource::PointLightShadowResource(uint32_t width, uint32_t height)
    : RenderResource()
{
    mShadowExtent = VkExtent2D{
        .width = RT::POINT_LIGHT_SHADOW_WIDTH,
        .height = RT::POINT_LIGHT_SHADOW_HEIGHT
    };
    CreateShadowCubeMap();
}

//-------------------------------------------------------------------------------------------------

MFA::PointLightShadowResource::~PointLightShadowResource()
{
    mShadowCubeMapList.clear();
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowResource::GetShadowCubeMap(RT::CommandRecordState const & recordState) const
{
    return GetShadowCubeMap(recordState.frameIndex);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowResource::GetShadowCubeMap(uint32_t const frameIndex) const
{
    return *mShadowCubeMapList[frameIndex];
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResource::OnResize()
{
}

//-------------------------------------------------------------------------------------------------

VkExtent2D MFA::PointLightShadowResource::GetShadowExtent() const
{
    return mShadowExtent;
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResource::CreateShadowCubeMap()
{
    mShadowCubeMapList.resize(RF::GetMaxFramesPerFlight());
    for (auto & shadowCubeMap : mShadowCubeMapList)
    {
        // TODO We can have cube array to render all lights in 1 pass
        shadowCubeMap = RF::CreateDepthImage(
            mShadowExtent,
            RT::CreateDepthImageOptions{
                .layerCount = 6 * RT::MAX_POINT_LIGHT_COUNT,
                .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                .imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            }
        );
    }
}

//-------------------------------------------------------------------------------------------------
