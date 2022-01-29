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

        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState);

        void OnResize() override;

    protected:

        void internalInit() override;

        void internalShutdown() override;

    private:

        void createRenderPass();
        
        void createFrameBuffers(VkExtent2D const & extent);

        [[nodiscard]]
        VkFramebuffer getFrameBuffer(RT::CommandRecordState const & drawPass) const;

        VkRenderPass mRenderPass {};
        std::vector<VkFramebuffer> mFrameBuffers {};

        DisplayRenderPass * mDisplayRenderPass = nullptr;

    };

}
