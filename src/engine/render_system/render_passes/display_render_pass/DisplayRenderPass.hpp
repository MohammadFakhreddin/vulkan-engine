#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_passes/RenderPass.hpp"
#include "engine/render_system/render_resources/display_render_resources/DisplayRenderResources.hpp"

namespace MFA
{

    class DisplayRenderPass final : public RenderPass
    {
    public:

        explicit DisplayRenderPass(std::shared_ptr<DisplayRenderResources> resources);
        ~DisplayRenderPass() override;

        DisplayRenderPass(DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass(DisplayRenderPass &&) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;
        
        void BeginRenderPass(RT::CommandRecordState & recordState);

        void EndRenderPass(RT::CommandRecordState & recordState);
        
        void NotifyDepthImageLayoutIsSet();
        
    private:


        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t imageIndex) const;

        void CreateFrameBuffers(VkExtent2D const & extent);

        void CreateRenderPass();
        
        void CreatePresentToDrawBarrier();

        void ClearDepthBufferIfNeeded(RT::CommandRecordState const & recordState);
        
        void UsePresentToDrawBarrier(RT::CommandRecordState const & recordState);

        VkRenderPass mVkDisplayRenderPass{};            // TODO Make this a renderType
        
        VkImageMemoryBarrier mPresentToDrawBarrier {};

        bool mIsDepthImageUndefined = true;

        std::shared_ptr<DisplayRenderResources> mResources{};

        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};

    };

}
