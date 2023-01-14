#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class DirectionalLightShadowResource;

    class DirectionalLightShadowRenderPass final : public RenderPass
    {
    public:

        explicit DirectionalLightShadowRenderPass(std::shared_ptr<DirectionalLightShadowResource> shadowResource);

        ~DirectionalLightShadowRenderPass() override;

        DirectionalLightShadowRenderPass(DirectionalLightShadowRenderPass const &) noexcept = delete;
        DirectionalLightShadowRenderPass(DirectionalLightShadowRenderPass &&) noexcept = delete;
        DirectionalLightShadowRenderPass & operator = (DirectionalLightShadowRenderPass const &) noexcept = delete;
        DirectionalLightShadowRenderPass & operator = (DirectionalLightShadowRenderPass &&) noexcept = delete;

        VkRenderPass GetVkRenderPass() override;

        // Appends required data for barrier to execute
        void BeginRenderPass(RT::CommandRecordState & recordState) override;

        void EndRenderPass(RT::CommandRecordState & recordState) override;
        
        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

        void CreateFrameBuffer();

    private:

        void CreateRenderPass();

        std::shared_ptr<DirectionalLightShadowResource> mShadowResource {};
        std::shared_ptr<RT::RenderPass> mRenderPass{};
        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};

    };
}
