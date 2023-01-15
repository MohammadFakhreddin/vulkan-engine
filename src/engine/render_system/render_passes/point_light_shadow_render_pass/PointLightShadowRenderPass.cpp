#include "PointLightShadowRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/RenderBackend.hpp"
#include "engine/render_system/render_resources/point_light_shadow/PointLightShadowResource.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    PointLightShadowRenderPass::PointLightShadowRenderPass(std::shared_ptr<PointLightShadowResource> pointLightShadowResource)
        : RenderPass()
        , mPointLightShadowResource(std::move(pointLightShadowResource))
    {
        CreateRenderPass();
    }

    //-------------------------------------------------------------------------------------------------

    PointLightShadowRenderPass::~PointLightShadowRenderPass()
    {
        mFrameBuffers.clear();
        mRenderPass.reset();
    }

    //-------------------------------------------------------------------------------------------------

    VkRenderPass PointLightShadowRenderPass::GetVkRenderPass()
    {
        return mRenderPass->vkRenderPass;
    }

    //-------------------------------------------------------------------------------------------------

    //void PointLightShadowRenderPass::Init()
    //{
    //    CreateRenderPass();

    //    // TODO We might need special sampler as well
    //    // Create sampler
    //    /*
    //    VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    //    sampler.magFilter = VK_FILTER_LINEAR;
    //    sampler.minFilter = VK_FILTER_LINEAR;
    //    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    //    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    //    sampler.addressModeV = sampler.addressModeU;
    //    sampler.addressModeW = sampler.addressModeU;
    //    sampler.mipLodBias = 0.0f;
    //    sampler.compareOp = VK_COMPARE_OP_NEVER;
    //    sampler.minLod = 0.0f;
    //    sampler.maxLod = cubeMap.mipLevels;
    //    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    //    sampler.maxAnisotropy = 1.0f;
    //    if (vulkanDevice->features.samplerAnisotropy)
    //    {
    //        sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
    //        sampler.anisotropyEnable = VK_TRUE;
    //    }
    //    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &cubeMap.sampler));
    //    */

    //}
    
    //-------------------------------------------------------------------------------------------------

    //void PointLightShadowRenderPass::PrepareRenderTargetForShading(
    //    RT::CommandRecordState const & recordState,
    //    PointLightShadowResourceCollection * renderTarget,
    //    std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    //) const
    //{
    //    MFA_ASSERT(renderTarget != nullptr);

    //    // Making color image ready for color attachment
    //    VkImageSubresourceRange const subResourceRange{
    //        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //        .baseMipLevel = 0,
    //        .levelCount = 1,
    //        .baseArrayLayer = 0,
    //        .layerCount = 6,
    //    };

    //    VkImageMemoryBarrier const pipelineBarrier{
    //        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //        .srcAccessMask = 0,
    //        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //        .image = renderTarget->GetShadowCubeMap(recordState).imageGroup.image,
    //        .subresourceRange = subResourceRange
    //    };
    //    outPipelineBarriers.emplace_back(pipelineBarrier);
    //}

    //-------------------------------------------------------------------------------------------------

    //void PointLightShadowRenderPass::PrepareRenderTargetForImageTransfer(
    //    RT::CommandRecordState const & recordState,
    //    PointLightShadowResourceCollection * renderTarget,
    //    std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    //) const
    //{
    //    
    //    {// Preparing cube-map for transfer destination
    //        VkImageSubresourceRange const subResourceRange{
    //            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //            .baseMipLevel = 0,
    //            .levelCount = 1,
    //            .baseArrayLayer = 0,
    //            .layerCount = 6,
    //        };

    //        VkImageMemoryBarrier const pipelineBarrier{
    //            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //            .srcAccessMask = 0,
    //            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //            .image = renderTarget->GetShadowCubeMap(recordState).imageGroup.image,
    //            .subresourceRange = subResourceRange
    //        };
    //        outPipelineBarriers.emplace_back(pipelineBarrier);

    //    }

    //    // Color attachment barrier
    //    for (int faceIndex = 0; faceIndex < CubeFaceCount; ++faceIndex)
    //    {
    //        VkImageSubresourceRange const subResourceRange{
    //            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //            .baseMipLevel = 0,
    //            .levelCount = 1,
    //            .baseArrayLayer = 0,
    //            .layerCount = 1,
    //        };

    //        VkImageMemoryBarrier const pipelineBarrier{
    //            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //            .srcAccessMask = 0,
    //            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //            .image = renderTarget->GetColorAttachmentImage(recordState, faceIndex).imageGroup.image,
    //            .subresourceRange = subResourceRange
    //        };
    //        outPipelineBarriers.emplace_back(pipelineBarrier);
    //    }
    //}

    //-------------------------------------------------------------------------------------------------

    //void PointLightShadowRenderPass::CopyColorAttachmentIntoCubeMap(
    //    RT::CommandRecordState const & recordState,
    //    PointLightShadowResourceCollection * renderTarget
    //)
    //{
    //    for (int faceIndex = 0; faceIndex < CubeFaceCount; ++faceIndex)
    //    {// Copy Color attachment image into DepthCubeMap
    //        auto const shadowExtend = VkExtent2D{
    //            .width = Scene::SHADOW_WIDTH,
    //            .height = Scene::SHADOW_HEIGHT
    //        };
    //        // Copy region for transfer from frame buffer to cube face
    //        VkImageCopy const copyRegion{
    //            .srcSubresource = {
    //                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //                .mipLevel = 0,
    //                .baseArrayLayer = 0,
    //                .layerCount = 1,
    //            },
    //            .srcOffset = { 0, 0, 0 },
    //            .dstSubresource {
    //                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //                .mipLevel = 0,
    //                .baseArrayLayer = static_cast<uint32_t>(faceIndex),
    //                .layerCount = 1,
    //            },
    //            .dstOffset = { 0, 0, 0 },
    //            .extent {
    //                .width = shadowExtend.width,
    //                .height = shadowExtend.height,
    //                .depth = 1,
    //            }
    //        };
    //        RB::CopyImage(
    //            RF::GetGraphicCommandBuffer(recordState),
    //            renderTarget->GetColorAttachmentImage(recordState, faceIndex).imageGroup.image,
    //            renderTarget->GetShadowCubeMap(recordState).imageGroup.image,
    //            copyRegion
    //        );
    //    }
    //}

    //-------------------------------------------------------------------------------------------------

    void PointLightShadowRenderPass::PrepareRenderTargetForSampling(
        RT::CommandRecordState const & recordState,
        PointLightShadowResource * renderTarget,
        bool const isUsed,
        std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    )
    {
        MFA_ASSERT(renderTarget != nullptr);

        // Preparing shadow cube map for shader sampling
        VkImageSubresourceRange const subResourceRange{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 6 * RT::MAX_POINT_LIGHT_COUNT,
        };

        VkImageMemoryBarrier const pipelineBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .oldLayout = isUsed ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = renderTarget->GetShadowCubeMap(recordState).imageGroup->image,
            .subresourceRange = subResourceRange
        };

        outPipelineBarriers.emplace_back(pipelineBarrier);
    }

    //-------------------------------------------------------------------------------------------------

    //void PointLightShadowRenderPass::PrepareUnUsedRenderTargetForSampling(
    //    RT::CommandRecordState const & recordState,
    //    PointLightShadowResourceCollection * renderTarget,
    //    std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    //) const
    //{
    //    MFA_ASSERT(renderTarget != nullptr);
    //    VkImageSubresourceRange const subResourceRange{
    //        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    //        .baseMipLevel = 0,
    //        .levelCount = 1,
    //        .baseArrayLayer = 0,
    //        .layerCount = 6 * Scene::MAX_VISIBLE_POINT_LIGHT_COUNT,
    //    };

    //    VkImageMemoryBarrier const pipelineBarrier{
    //        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //        .srcAccessMask = 0,
    //        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //        .image = renderTarget->GetShadowCubeMap(recordState).imageGroup.image,
    //        .subresourceRange = subResourceRange
    //    };

    //    outPipelineBarriers.emplace_back(pipelineBarrier);
    //}

    //-------------------------------------------------------------------------------------------------
    // RenderTarget = FrameBuffer + RenderPass! Fix the naming
    void PointLightShadowRenderPass::BeginRenderPass(RT::CommandRecordState & recordState)
    {

        RenderPass::BeginRenderPass(recordState);

        // Shadow map generation
        auto const shadowExtend = VkExtent2D{
            .width = RT::POINT_LIGHT_SHADOW_WIDTH,
            .height = RT::POINT_LIGHT_SHADOW_HEIGHT
        };
        
        RF::AssignViewportAndScissorToCommandBuffer(recordState.commandBuffer, shadowExtend);

        std::vector<VkClearValue> clearValues{};
        clearValues.resize(1);
        clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0 };

        RF::BeginRenderPass(
            recordState.commandBuffer,
            mRenderPass->vkRenderPass,
            GetFrameBuffer(recordState),
            shadowExtend,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        );
    }

    //-------------------------------------------------------------------------------------------------

    void PointLightShadowRenderPass::EndRenderPass(RT::CommandRecordState & recordState)
    {
        RenderPass::EndRenderPass(recordState);

        RF::EndRenderPass(recordState.commandBuffer);
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer PointLightShadowRenderPass::GetFrameBuffer(RT::CommandRecordState const& recordState) const
    {
        return GetFrameBuffer(recordState.frameIndex);
    }

    //-------------------------------------------------------------------------------------------------

    VkFramebuffer PointLightShadowRenderPass::GetFrameBuffer(uint32_t const frameIndex) const
    {
        return mFrameBuffers[frameIndex];
    }

    //-------------------------------------------------------------------------------------------------

    void PointLightShadowRenderPass::CreateRenderPass()
    {
        std::vector<VkAttachmentDescription> attachments{};

        // Depth attachment
        attachments.emplace_back(VkAttachmentDescription{
            .format = RF::GetDepthFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        });

        VkAttachmentReference depthReference{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        std::vector<VkSubpassDescription> subPasses{
            VkSubpassDescription {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 0,
                .pDepthStencilAttachment = &depthReference,
            }
        };

        mRenderPass = std::make_shared<RT::RenderPass>(RF::CreateRenderPass(
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            subPasses.data(),
            static_cast<uint32_t>(subPasses.size()),
            nullptr,
            0
        ));
    }

    //-------------------------------------------------------------------------------------------------

    void PointLightShadowRenderPass::CreateFrameBuffer()
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
            std::vector<VkImageView> const attachments{ mPointLightShadowResource->GetShadowCubeMap(i).imageView->imageView, };

            mFrameBuffers[i] = RF::CreateFrameBuffer(
                mRenderPass->vkRenderPass,
                attachments.data(),
                static_cast<uint32_t>(attachments.size()),
                mPointLightShadowResource->GetShadowExtent(),
                6 * RT::MAX_POINT_LIGHT_COUNT
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
