#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA {

class PointLightShadowResourceCollection;
class DisplayRenderPass;

class PointLightShadowRenderPass final : public RenderPass {
public:
    
    PointLightShadowRenderPass();

    ~PointLightShadowRenderPass() override;

    PointLightShadowRenderPass (PointLightShadowRenderPass const &) noexcept = delete;
    PointLightShadowRenderPass (PointLightShadowRenderPass &&) noexcept = delete;
    PointLightShadowRenderPass & operator = (PointLightShadowRenderPass const &) noexcept = delete;
    PointLightShadowRenderPass & operator = (PointLightShadowRenderPass &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    // Appends required data for barrier to execute
    //void PrepareRenderTargetForShading(
    //    RT::CommandRecordState const & recordState,
    //    PointLightShadowResourceCollection * renderTarget,
    //    std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    //) const;
    
    // Appends required data for barrier to execute
    void PrepareUsedRenderTargetForSampling(
        RT::CommandRecordState const & recordState,
        PointLightShadowResourceCollection * renderTarget,
        std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    ) const;

    // Appends required data for barrier to execute
    void PrepareUnUsedRenderTargetForSampling(
        RT::CommandRecordState const & recordState,
        PointLightShadowResourceCollection * renderTarget,
        std::vector<VkImageMemoryBarrier> & outPipelineBarriers
    ) const;

    // Appends required data for barrier to execute
    void BeginRenderPass(
        RT::CommandRecordState& recordState,
        const PointLightShadowResourceCollection& renderTarget
    );

    void EndRenderPass(RT::CommandRecordState & drawPass);

protected:

    void internalInit() override;

    void internalShutdown() override;
    
private:

    void createRenderPass();

    VkRenderPass mVkRenderPass {};

};

}
