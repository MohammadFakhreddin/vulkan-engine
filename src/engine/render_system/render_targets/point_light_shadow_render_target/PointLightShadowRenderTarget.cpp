#include "PointLightShadowRenderTarget.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowRenderTarget::Init(VkRenderPass renderPass)
{
    static constexpr auto shadowExtend = VkExtent2D{
        .width = Scene::SHADOW_WIDTH,
        .height = Scene::SHADOW_HEIGHT
    };
    createDepthCubeMap(shadowExtend);
    createDepthImage(shadowExtend);
    createFrameBuffer(shadowExtend, renderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowRenderTarget::Shutdown()
{
    RF::DestroyFrameBuffers(static_cast<uint32_t>(mFrameBuffers.size()), mFrameBuffers.data());
    mFrameBuffers.clear();

    for (auto & depthCubeMap : mDepthCubeMapList)
    {
        RF::DestroyDepthImage(depthCubeMap);
    }
    mDepthCubeMapList.clear();

    for (auto & depthImage : mDepthImageList)
    {
        RF::DestroyDepthImage(depthImage);
    }
    mDepthImageList.clear();
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowRenderTarget::GetDepthCubeMap(RT::CommandRecordState const & drawPass) const
{
    return GetDepthCubeMap(drawPass.frameIndex);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowRenderTarget::GetDepthCubeMap(uint32_t const frameIndex) const
{
    return mDepthCubeMapList[frameIndex];
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DepthImageGroup const & MFA::PointLightShadowRenderTarget::GetDepthImage(RT::CommandRecordState const & drawPass) const
{
    return GetDepthImage(drawPass.frameIndex);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DepthImageGroup const & MFA::PointLightShadowRenderTarget::GetDepthImage(uint32_t const frameIndex) const
{
    return mDepthImageList[frameIndex];
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::PointLightShadowRenderTarget::GetFrameBuffer(RT::CommandRecordState const & drawPass) const
{
    return GetFrameBuffer(drawPass.frameIndex);
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::PointLightShadowRenderTarget::GetFrameBuffer(uint32_t const frameIndex) const
{
    return mFrameBuffers[frameIndex];
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowRenderTarget::createDepthCubeMap(VkExtent2D const & shadowExtent)
{
    mDepthCubeMapList.resize(RF::GetMaxFramesPerFlight());
    for (auto & depthCubeMap : mDepthCubeMapList)
    {
        depthCubeMap = RF::CreateDepthImage(shadowExtent, RT::CreateDepthImageOptions{
            .layerCount = 6, // * light count
            .usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
            .imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
        });
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowRenderTarget::createDepthImage(VkExtent2D const & shadowExtent)
{
    mDepthImageList.resize(RF::GetMaxFramesPerFlight());
    for (auto & depthImage : mDepthImageList)
    {
        depthImage = RF::CreateDepthImage(shadowExtent, RT::CreateDepthImageOptions{
            .layerCount = 1,
            .usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        });
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowRenderTarget::createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass)
{
    // Note: This comment is useful though does not match 100% with my code
    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetMaxFramesPerFlight());
    for (uint32_t i = 0; i < static_cast<uint32_t>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments = {
            mDepthImageList[i].imageView
        };

        mFrameBuffers[i] = RF::CreateFrameBuffer(
            renderPass,
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            shadowExtent,
            1   // * lightCount
        );
    }
}

//-------------------------------------------------------------------------------------------------
