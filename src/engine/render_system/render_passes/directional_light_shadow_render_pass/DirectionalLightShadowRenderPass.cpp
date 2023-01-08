#include "DirectionalLightShadowRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_resources/directional_light_shadow_resources/DirectionalLightShadowResources.hpp"

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightShadowRenderPass::DirectionalLightShadowRenderPass() = default;

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightShadowRenderPass::~DirectionalLightShadowRenderPass() = default;

//-------------------------------------------------------------------------------------------------

VkRenderPass MFA::DirectionalLightShadowRenderPass::GetVkRenderPass()
{
    return mVkRenderPass;
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::PrepareRenderTargetForSampling(
    RT::CommandRecordState const & recordState,
    DirectionalLightShadowResources * renderTarget,
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
        .layerCount = RT::MAX_DIRECTIONAL_LIGHT_COUNT,
    };

    VkImageMemoryBarrier const pipelineBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        .oldLayout = isUsed ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = renderTarget->GetShadowMap(recordState).imageGroup->image,
        .subresourceRange = subResourceRange
    };

    outPipelineBarriers.emplace_back(pipelineBarrier);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::BeginRenderPass(
    RT::CommandRecordState & recordState,
    const DirectionalLightShadowResources & renderTarget
)
{
    RenderPass::BeginRenderPass(recordState);

    // Shadow map generation
    auto const shadowExtend = VkExtent2D{
        .width = RT::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH,
        .height = RT::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT
    };

    auto * commandBuffer = recordState.commandBuffer;
    MFA_ASSERT(commandBuffer != nullptr);

    RF::AssignViewportAndScissorToCommandBuffer(commandBuffer, shadowExtend);

    std::vector<VkClearValue> clearValues{};
    clearValues.resize(1);
    clearValues[0].depthStencil = { .depth = 1.0f, .stencil = 0 };

    RF::BeginRenderPass(
        commandBuffer,
        mVkRenderPass,
        renderTarget.GetFrameBuffer(recordState),
        shadowExtend,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::EndRenderPass(RT::CommandRecordState & recordState)
{
    RenderPass::EndRenderPass(recordState);

    RF::EndRenderPass(recordState.commandBuffer);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::Init()
{
    createRenderPass();
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::Shutdown()
{
    RF::DestroyRenderPass(mVkRenderPass);
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::createRenderPass()
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

    mVkRenderPass = RF::CreateRenderPass(
        attachments.data(),
        static_cast<uint32_t>(attachments.size()),
        subPasses.data(),
        static_cast<uint32_t>(subPasses.size()),
        nullptr,
        0
    );
}

//-------------------------------------------------------------------------------------------------
