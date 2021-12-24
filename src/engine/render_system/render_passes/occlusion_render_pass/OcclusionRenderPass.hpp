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

        void createDepthImage(VkExtent2D const & extent);
        
        void copyDisplayPassDepthBuffer(
            RT::CommandRecordState const & recordState,
            VkExtent2D const & extent2D
        ) const;

        [[nodiscard]]
        VkFramebuffer getFrameBuffer(RT::CommandRecordState const & drawPass) const;

        uint32_t mImageWidth = 0;
        uint32_t mImageHeight = 0;

        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList {};
        VkRenderPass mRenderPass {};
        std::vector<VkFramebuffer> mFrameBuffers {};
    };

}
