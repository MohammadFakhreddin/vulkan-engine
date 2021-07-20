#pragma once
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;

class RenderPass {

public:

    virtual ~RenderPass() = default;

    void Init();

    void Shutdown();

    RF::DrawPass Begin();

    void End(RF::DrawPass & drawPass);

    void OnResize();

    virtual VkRenderPass GetVkRenderPass() = 0;

protected:

    virtual void internalInit() = 0;

    virtual void internalShutdown() = 0;

    virtual RF::DrawPass internalBegin() = 0;

    virtual void internalEnd(RF::DrawPass & drawPass) = 0;

    virtual void internalResize() = 0;

};

}
