#pragma once

#include "engine/render_system/RenderTypes.hpp"

namespace MFA
{
    class DirectionalLightShadowResources
    {
    public:

        void Init(VkRenderPass renderPass);

        void Shutdown();

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowMap(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowMap(uint32_t frameIndex) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & drawPass) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

    private:

        static constexpr uint32_t CubeFaceCount = 6;

        void createShadowMap(VkExtent2D const & shadowExtent);

        void createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass);

        std::vector<VkFramebuffer> mFrameBuffers{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mShadowMaps{};
    };
}
