#include "PointLightShadowResourceCollection.hpp"

#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/Scene.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResourceCollection::Init(VkRenderPass renderPass)
{
    static constexpr auto shadowExtend = VkExtent2D{
        .width = Scene::SHADOW_WIDTH,
        .height = Scene::SHADOW_HEIGHT
    };
    createShadowCubeMap(shadowExtend);
    createFrameBuffer(shadowExtend, renderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResourceCollection::Shutdown()
{
    RF::DestroyFrameBuffers(static_cast<uint32_t>(mFrameBuffers.size()), mFrameBuffers.data());
    mFrameBuffers.clear();

    for (auto & shadowCubeMap : mShadowCubeMapList)
    {
        RF::DestroyDepthImage(shadowCubeMap);
    }
    mShadowCubeMapList.clear();
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowResourceCollection::GetShadowCubeMap(RT::CommandRecordState const & recordState) const
{
    return GetShadowCubeMap(recordState.frameIndex);
}

//-------------------------------------------------------------------------------------------------

MFA::RT::DepthImageGroup const & MFA::PointLightShadowResourceCollection::GetShadowCubeMap(uint32_t const frameIndex) const
{
    return mShadowCubeMapList[frameIndex];
}

//-------------------------------------------------------------------------------------------------

//MFA::RenderTypes::DepthImageGroup const & MFA::PointLightShadowResourceCollection::GetDepthAttachmentImage(RT::CommandRecordState const & recordState) const
//{
//    return GetDepthAttachmentImage(recordState.frameIndex);
//}

//-------------------------------------------------------------------------------------------------

//MFA::RenderTypes::DepthImageGroup const & MFA::PointLightShadowResourceCollection::GetDepthAttachmentImage(uint32_t const frameIndex) const
//{
//    return mDepthAttachmentImageList[frameIndex];
//}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::PointLightShadowResourceCollection::GetFrameBuffer(RT::CommandRecordState const & drawPass) const
{
    return GetFrameBuffer(drawPass.frameIndex);
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::PointLightShadowResourceCollection::GetFrameBuffer(uint32_t const frameIndex) const
{
    return mFrameBuffers[frameIndex];
}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResourceCollection::createShadowCubeMap(VkExtent2D const & shadowExtent)
{
    mShadowCubeMapList.resize(RF::GetMaxFramesPerFlight());
    for (auto & shadowCubeMap : mShadowCubeMapList)
    {
        // TODO We can have cube array to render all lights in 1 pass
        shadowCubeMap = RF::CreateDepthImage(
            shadowExtent,
            RT::CreateDepthImageOptions{
                .layerCount = 6 * Scene::MAX_VISIBLE_POINT_LIGHT_COUNT,
                .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                .imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            }
        );
    }
}

//-------------------------------------------------------------------------------------------------

//void MFA::PointLightShadowResourceCollection::createDepthAttachmentImage(VkExtent2D const & shadowExtent)
//{
//    mDepthAttachmentImageList.resize(RF::GetMaxFramesPerFlight());
//    for (auto & depthImage : mDepthAttachmentImageList)
//    {
//        depthImage = RF::CreateDepthImage(shadowExtent, RT::CreateDepthImageOptions{
//            .layerCount = 6,
//            .usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
//            .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
//            .imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
//        });
//    }
//}

//-------------------------------------------------------------------------------------------------

void MFA::PointLightShadowResourceCollection::createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass)
{
    // Note: This comment is useful though does not match 100% with my code
    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetMaxFramesPerFlight());  // Per face index
    for (uint32_t i = 0; i < static_cast<uint32_t>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments = {
            mShadowCubeMapList[i].imageView,
        };

        mFrameBuffers[i] = RF::CreateFrameBuffer(
            renderPass,
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            shadowExtent,
            6 * Scene::MAX_VISIBLE_POINT_LIGHT_COUNT
        );
    }
}

//-------------------------------------------------------------------------------------------------
