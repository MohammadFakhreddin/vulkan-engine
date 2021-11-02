#pragma once

#include "engine/render_system/render_passes/RenderPass.hpp"

namespace MFA {

class PointLightShadowRenderTarget;
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

    void PrepareRenderTargetForTransferDestination(
        RT::CommandRecordState const & drawPass,
        PointLightShadowRenderTarget * renderTarget
    );

    void PrepareRenderTargetForSampling(RT::CommandRecordState const & recordPass);

    void PrepareRenderTargetForSampling(RT::CommandRecordState const & recordState, PointLightShadowRenderTarget * renderTarget) const;

    void SetNextPassParams(int faceIndex);

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RT::CommandRecordState & recordState) override;

    void internalEndRenderPass(RT::CommandRecordState & drawPass) override;

private:

    void createRenderPass();

    VkRenderPass mVkRenderPass {};

    int mFaceIndex = -1;

    PointLightShadowRenderTarget * mAttachedRenderTarget = nullptr;
};

}
