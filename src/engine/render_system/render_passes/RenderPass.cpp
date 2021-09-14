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

void RenderPass::BeginRenderPass(RT::DrawPass & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == false);
    mIsRenderPassActive = true;
    internalBeginRenderPass(drawPass);
}

void RenderPass::EndRenderPass(RT::DrawPass & drawPass) {
    MFA_ASSERT(mIsRenderPassActive == true);
    internalEndRenderPass(drawPass);
    mIsRenderPassActive = false;
}

void RenderPass::OnResize() {
    internalResize();
}

}
