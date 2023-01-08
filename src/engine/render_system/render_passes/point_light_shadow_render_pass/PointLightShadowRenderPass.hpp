#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class PointLightShadowResources;
    class DisplayRenderPass;

    class PointLightShadowRenderPass final : public RenderPass
    {
    public:

        PointLightShadowRenderPass();

        ~PointLightShadowRenderPass() override;

        PointLightShadowRenderPass(PointLightShadowRenderPass const &) noexcept = delete;
        PointLightShadowRenderPass(PointLightShadowRenderPass &&) noexcept = delete;
        PointLightShadowRenderPass & operator = (PointLightShadowRenderPass const &) noexcept = delete;
        PointLightShadowRenderPass & operator = (PointLightShadowRenderPass &&) noexcept = delete;

        VkRenderPass GetVkRenderPass() override;

        // Appends required data for barrier to execute
        static void PrepareRenderTargetForSampling(
            RT::CommandRecordState const & recordState,
            PointLightShadowResources * renderTarget,
            bool isUsed,
            std::vector<VkImageMemoryBarrier> & outPipelineBarriers
        );

        // Appends required data for barrier to execute
        void BeginRenderPass(
            RT::CommandRecordState & recordState,
            const PointLightShadowResources & renderTarget
        );

        void EndRenderPass(RT::CommandRecordState & recordState) override;

        void Init();

        void Shutdown();

    private:

        void createRenderPass();

        VkRenderPass mVkRenderPass{};

    };

}
