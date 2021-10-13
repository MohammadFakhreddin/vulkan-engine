#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class OcclusionRenderPass final : public RenderPass
    {
    public:

        explicit OcclusionRenderPass();

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

        uint32_t mImageWidth;
        uint32_t mImageHeight;

        VkRenderPass mRenderPass {};
        std::vector<VkFramebuffer> mFrameBuffers {};
        std::vector<RT::ColorImageGroup> mColorImages {};
        std::vector<RT::DepthImageGroup> mDepthImages {};

    };

}
