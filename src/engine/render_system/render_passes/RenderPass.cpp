#include "RenderPass.hpp"

namespace MFA {

void RenderPass::Init() {
    internalInit();
}

void RenderPass::Shutdown() {
    internalShutdown();
}

void RenderPass::BeginRenderPass(RF::DrawPass const & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == false);
    mIsRenderPassActive = true;
    internalBeginRenderPass(drawPass);
}

void RenderPass::EndRenderPass(RF::DrawPass const & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == true);
    internalEndRenderPass(drawPass);
    mIsRenderPassActive = false;
}

void RenderPass::OnResize() {
    internalResize();
}

}
