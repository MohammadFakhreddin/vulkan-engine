#include "RenderPass.hpp"

namespace MFA {

void RenderPass::Init() {
    internalInit();
}

void RenderPass::Shutdown() {
    internalShutdown();
}

RF::DrawPass RenderPass::Begin() {
    return internalBegin();
}

void RenderPass::End(RF::DrawPass & drawPass) {
    internalEnd(drawPass);
}

void RenderPass::OnResize() {
    internalResize();
}

}
