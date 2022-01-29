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
        static void PrepareRenderTargetForSampling(
            RT::CommandRecordState const & recordState,
            DirectionalLightShadowResources * renderTarget,
            bool isUsed,
            std::vector<VkImageMemoryBarrier> & outPipelineBarriers
        );

        // Appends required data for barrier to execute
        void BeginRenderPass(
            RT::CommandRecordState & recordState,
            const DirectionalLightShadowResources & renderTarget
        );

        void EndRenderPass(RT::CommandRecordState & recordState);

    protected:

        void internalInit() override;

        void internalShutdown() override;

    private:

        void createRenderPass();

        VkRenderPass mVkRenderPass{};

    };
}
