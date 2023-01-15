#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/render_resources/point_light_shadow/PointLightShadowResource.hpp"

namespace MFA
{

    class PointLightShadowRenderPass final : public RenderPass
    {
    public:

        explicit PointLightShadowRenderPass(std::shared_ptr<PointLightShadowResource> pointLightShadowResource);

        ~PointLightShadowRenderPass() override;

        PointLightShadowRenderPass(PointLightShadowRenderPass const &) noexcept = delete;
        PointLightShadowRenderPass(PointLightShadowRenderPass &&) noexcept = delete;
        PointLightShadowRenderPass & operator = (PointLightShadowRenderPass const &) noexcept = delete;
        PointLightShadowRenderPass & operator = (PointLightShadowRenderPass &&) noexcept = delete;

        VkRenderPass GetVkRenderPass() override;

        // Appends required data for barrier to execute
        static void PrepareRenderTargetForSampling(
            RT::CommandRecordState const & recordState,
            PointLightShadowResource * renderTarget,
            bool isUsed,
            std::vector<VkImageMemoryBarrier> & outPipelineBarriers
        );

        // Appends required data for barrier to execute
        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState) override;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

    private:

        void CreateRenderPass();

        void CreateFrameBuffer();

        std::shared_ptr<PointLightShadowResource> mPointLightShadowResource {};

        std::shared_ptr<RT::RenderPass> mRenderPass{};

        std::vector<VkFramebuffer> mFrameBuffers{};

    };

}
