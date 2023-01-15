#include "DirectionalLightShadowRenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/render_system/render_resources/directional_light_shadow/DirectionalLightShadowResource.hpp"

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightShadowRenderPass::DirectionalLightShadowRenderPass(
    std::shared_ptr<DirectionalLightShadowResource> shadowResource
)
    : RenderPass()
    , mShadowResource(std::move(shadowResource))
{
    CreateRenderPass();
    CreateFrameBuffer();
}

//-------------------------------------------------------------------------------------------------

MFA::DirectionalLightShadowRenderPass::~DirectionalLightShadowRenderPass()
{
    mFrameBuffers.clear();
    mRenderPass.reset();
}

//-------------------------------------------------------------------------------------------------

VkRenderPass MFA::DirectionalLightShadowRenderPass::GetVkRenderPass()
{
    return mRenderPass->vkRenderPass;
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::BeginRenderPass(RT::CommandRecordState & recordState)
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
        mRenderPass->vkRenderPass,
        GetFrameBuffer(recordState),
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

VkFramebuffer MFA::DirectionalLightShadowRenderPass::GetFrameBuffer(RT::CommandRecordState const& recordState) const
{
    return GetFrameBuffer(recordState.frameIndex);
}

//-------------------------------------------------------------------------------------------------

VkFramebuffer MFA::DirectionalLightShadowRenderPass::GetFrameBuffer(uint32_t const frameIndex) const
{
    return mFrameBuffers[frameIndex]->framebuffer;
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::CreateFrameBuffer()
{
    mFrameBuffers.clear();
    mFrameBuffers.resize(RF::GetMaxFramesPerFlight());  // Per face index
    for (uint32_t i = 0; i < static_cast<uint32_t>(mFrameBuffers.size()); ++i)
    {
        std::vector<VkImageView> const attachments{ mShadowResource->GetShadowMap(i).imageView->imageView };

        mFrameBuffers[i] = std::make_shared<RT::FrameBuffer>(RF::CreateFrameBuffer(
            mRenderPass->vkRenderPass,
            attachments.data(),
            static_cast<uint32_t>(attachments.size()),
            mShadowResource->GetShadowExtend(),
            RT::MAX_DIRECTIONAL_LIGHT_COUNT
        ));
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::DirectionalLightShadowRenderPass::CreateRenderPass()
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
