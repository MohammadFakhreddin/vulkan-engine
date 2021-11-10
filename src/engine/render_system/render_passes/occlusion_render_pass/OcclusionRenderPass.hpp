#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA
{

    class OcclusionRenderPass final : public RenderPass
    {
    public:

        explicit OcclusionRenderPass();

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
        VkFramebuffer getFrameBuffer(RT::CommandRecordState const & drawPass);

        uint32_t mImageWidth = 0;
        uint32_t mImageHeight = 0;

        VkRenderPass mRenderPass {};
        std::vector<VkFramebuffer> mFrameBuffers {};
    };

}
