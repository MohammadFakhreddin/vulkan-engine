#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <vector>

namespace MFA {

class PointLightShadowRenderTarget {
public:

    void Init(VkRenderPass renderPass);

    void Shutdown();
    
    [[nodiscard]]
    RT::DepthImageGroup const & GetDepthCubeMap(RT::CommandRecordState const & drawPass) const;

    [[nodiscard]]
    RT::DepthImageGroup const & GetDepthCubeMap(uint32_t frameIndex) const;

    [[nodiscard]]
    RT::DepthImageGroup const & GetDepthImage(RT::CommandRecordState const & drawPass) const;

    [[nodiscard]]
    RT::DepthImageGroup const & GetDepthImage(uint32_t frameIndex) const;

    [[nodiscard]]
    VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & drawPass) const;

    [[nodiscard]]
    VkFramebuffer GetFrameBuffer(uint32_t frameIndex) const;

private:

    void createDepthCubeMap(VkExtent2D const & shadowExtent);

    void createDepthImage(VkExtent2D const & shadowExtent);

    void createFrameBuffer(VkExtent2D const & shadowExtent, VkRenderPass renderPass);

    std::vector<VkFramebuffer> mFrameBuffers {};
    
    std::vector<RT::DepthImageGroup> mDepthCubeMapList {};                  // Used as shader attachment
    std::vector<RT::DepthImageGroup> mDepthImageList {};                    // Used as render target of pass (We copy the result to each face of depth cube)

};

}
