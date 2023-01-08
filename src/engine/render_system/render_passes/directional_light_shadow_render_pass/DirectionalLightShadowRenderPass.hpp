#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class DirectionalLightShadowResources;

    class DirectionalLightShadowRenderPass final : public RenderPass
    {
    public:
        DirectionalLightShadowRenderPass();

        ~DirectionalLightShadowRenderPass() override;

        DirectionalLightShadowRenderPass(DirectionalLightShadowRenderPass const &) noexcept = delete;
        DirectionalLightShadowRenderPass(DirectionalLightShadowRenderPass &&) noexcept = delete;
        DirectionalLightShadowRenderPass & operator = (DirectionalLightShadowRenderPass const &) noexcept = delete;
        DirectionalLightShadowRenderPass & operator = (DirectionalLightShadowRenderPass &&) noexcept = delete;

        VkRenderPass GetVkRenderPass() override;

        // Appends required data for barrier to execute
        void BeginRenderPass(RT::CommandRecordState & recordState) override;

        void EndRenderPass(RT::CommandRecordState & recordState) override;

        void Init(std::shared_ptr<DirectionalLightShadowResources> resources);

        void Shutdown();

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

        void CreateFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass);

    private:

        void createRenderPass();

        VkRenderPass mVkRenderPass{};

        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};

    };
}
