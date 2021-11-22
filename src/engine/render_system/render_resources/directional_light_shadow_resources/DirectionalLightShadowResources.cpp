#include "DirectionalLightShadowResources.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowResources::Init(VkRenderPass renderPass)
{
    static constexpr auto shadowExtend = VkExtent2D{
        .width = Scene::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH,
        .height = Scene::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT
    };
    createShadowMap(shadowExtend);
    createFrameBuffer(shadowExtend, renderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowResources::Shutdown()
{
    RF::DestroyFrameBuffers(static_cast<uint32_t>(mFrameBuffers.size()), mFrameBuffers.data());
    mFrameBuffers.clear();

    for (auto & shadowMap : mShadowMaps)
    {
        RF::DestroyDepthImage(shadowMap);
    }
    mShadowMaps.clear();
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DepthImageGroup const & MFA::DirectionalLightShadowResources::GetShadowMap(RT::CommandRecordState const & recordState) const
{
    return GetShadowMap(recordState.frameIndex);
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DepthImageGroup const & MFA::DirectionalLightShadowResources::GetShadowMap(uint32_t const frameIndex) const
{
    return mShadowMaps[frameIndex];
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::DirectionalLightShadowResources::GetFrameBuffer(RT::CommandRecordState const & drawPass) const
{
    return GetFrameBuffer(drawPass.frameIndex);
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::DirectionalLightShadowResources::GetFrameBuffer(uint32_t const frameIndex) const
{
    return mFrameBuffers[frameIndex];
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowResources::createShadowMap(VkExtent2D const & shadowExtent)
{
    mShadowMaps.resize(RF::GetMaxFramesPerFlight());
    for (auto & shadowMap : mShadowMaps)
    {
        shadowMap = RF::CreateDepthImage(
            shadowExtent,
            RT::CreateDepthImageOptions{
                .layerCount = Scene::MAX_DIRECTIONAL_LIGHT_COUNT,
                .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .imageType = VK_IMAGE_TYPE_2D
            }
        );
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowResources::createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass)
{
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetMaxFramesPerFlight());  // Per face index
    for (uint32_t i = 0; i < static_cast<uint32_t>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments = {
            mShadowMaps[i].imageView,
        };

        mFrameBuffers[i] = RF::CreateFrameBuffer(
            renderPass,
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            shadowExtent,
            Scene::MAX_DIRECTIONAL_LIGHT_COUNT
        );
    }
}

//-------------------------------------------------------------------------------------------------
