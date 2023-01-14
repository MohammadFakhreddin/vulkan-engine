#include "DirectionalLightShadowResource.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderTypes.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    DirectionalLightShadowResource::DirectionalLightShadowResource(uint32_t width, uint32_t height)
    {
        mShadowExtend = VkExtent2D{
            .width = width,
            .height = height
        };
        CreateShadowMap();
    }

    //-------------------------------------------------------------------------------------------------

    DirectionalLightShadowResource::~DirectionalLightShadowResource()
    {
        mShadowMaps.clear();
    }

    //-------------------------------------------------------------------------------------------------

    RT::DepthImageGroup const & DirectionalLightShadowResource::GetShadowMap(RT::CommandRecordState const & recordState) const
    {
        return GetShadowMap(recordState.frameIndex);
    }

    //-------------------------------------------------------------------------------------------------

    RT::DepthImageGroup const & DirectionalLightShadowResource::GetShadowMap(uint32_t const frameIndex) const
    {
        return *mShadowMaps[frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void DirectionalLightShadowResource::PrepareForSampling(
        RT::CommandRecordState const & recordState,
        bool isUsed,
        std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    )
    {
        // Preparing shadow cube map for shader sampling
        VkImageSubresourceRange const subResourceRange{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = RT::MAX_DIRECTIONAL_LIGHT_COUNT,
        };

        auto shadowMap = this->GetShadowMap(recordState).imageGroup->image;

        VkImageMemoryBarrier const pipelineBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = isUsed ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = GetShadowMap(recordState).imageGroup->image,
            .subresourceRange = subResourceRange
        };

        outPipelineBarriers.emplace_back(pipelineBarrier);
    }

    //-------------------------------------------------------------------------------------------------

    void DirectionalLightShadowResource::OnResize()
    {}

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DirectionalLightShadowResource::GetFrameBuffer(RT::CommandRecordState const & recordState) const
    {
        return GetFrameBuffer(recordState.frameIndex);
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer DirectionalLightShadowResource::GetFrameBuffer(uint32_t const frameIndex) const
    {
        return mFrameBuffers[frameIndex]->framebuffer;
    }

    //-------------------------------------------------------------------------------------------------

    void DirectionalLightShadowResource::CreateShadowMap()
    {
        mShadowMaps.resize(RF::GetMaxFramesPerFlight());
        for (auto & shadowMap : mShadowMaps)
        {
            shadowMap = RF::CreateDepthImage(
                mShadowExtend,
                RT::CreateDepthImageOptions{
                    .layerCount = RT::MAX_DIRECTIONAL_LIGHT_COUNT,
                    .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                    .imageType = VK_IMAGE_TYPE_2D
                }
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
