#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <vector>

namespace MFA
{

    class PointLightShadowResources
    {
    public:

        void Init(VkRenderPass renderPass);

        void Shutdown();

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowCubeMap(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowCubeMap(uint32_t frameIndex) const;
        
        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & drawPass) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

    private:

        static constexpr uint32_t CubeFaceCount = 6;

        void createShadowCubeMap(VkExtent2D const & shadowExtent);
        
        void createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass);

        std::vector<VkFramebuffer> mFrameBuffers{};
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mShadowCubeMapList{};
    
    };

}
