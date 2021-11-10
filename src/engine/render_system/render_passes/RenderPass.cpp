#include "RenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"

namespace MFA {

void RenderPass::Init() {
    internalInit();
}

void RenderPass::Shutdown() {
    internalShutdown();
}

void RenderPass::BeginRenderPass(RT::CommandRecordState & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == false);
    mIsRenderPassActive = true;
    drawPass.renderPass = this;
}

void RenderPass::EndRenderPass(RT::CommandRecordState & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == true);
    drawPass.renderPass = nullptr;
    mIsRenderPassActive = false;
}

void RenderPass::OnResize() {}

}
