#include "RenderPass.hpp"

namespace MFA {

void RenderPass::Init() {
    internalInit();
}

void RenderPass::Shutdown() {
    internalShutdown();
}

void RenderPass::BeginRenderPass(RF::DrawPass const & drawPass) {
    internalBeginRenderPass(drawPass);
}

void RenderPass::EndRenderPass(RF::DrawPass const & drawPass) {
    internalEndRenderPass(drawPass);
}

void RenderPass::OnResize() {
    internalResize();
}

}
