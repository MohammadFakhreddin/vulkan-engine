#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{
    class DisplayRenderPass;

    class DepthPrePass final : public RenderPass
    {
    public:
        DepthPrePass() = default;
        ~DepthPrePass() override = default;

        DepthPrePass(DepthPrePass const &) noexcept = delete;
        DepthPrePass(DepthPrePass &&) noexcept = delete;
        DepthPrePass & operator = (DepthPrePass const &) noexcept = delete;
        DepthPrePass & operator = (DepthPrePass &&) noexcept = delete;

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;

    protected:

        void internalInit() override;

        void internalShutdown() override;

        void internalBeginRenderPass(RT::CommandRecordState & drawPass) override;

        void internalEndRenderPass(RT::CommandRecordState & drawPass) override;

        void internalResize() override;

    private:

        void createRenderPass();
        
        void createFrameBuffers(VkExtent2D const & extent);

        [[nodiscard]]
        VkFramebuffer getFrameBuffer(RT::CommandRecordState const & drawPass);

        VkRenderPass mRenderPass {};
        std::vector<VkFramebuffer> mFrameBuffers {};
        
    };

}
