#pragma once
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

namespace RF = RenderFrontend;

class RenderPass {

public:

    virtual ~RenderPass() = default;

    void Init();

    void Shutdown();

    void BeginRenderPass(RF::DrawPass const & drawPass);

    void EndRenderPass(RF::DrawPass const & drawPass);

    void OnResize();

    virtual VkRenderPass GetVkRenderPass() = 0;

protected:

    virtual void internalInit() = 0;

    virtual void internalShutdown() = 0;

    virtual void internalBeginRenderPass(RF::DrawPass const & drawPass) = 0;

    virtual void internalEndRenderPass(RF::DrawPass const & drawPass) = 0;

    virtual void internalResize() = 0;

    [[nodiscard]]
    bool getIsRenderPassActive() {
        return mIsRenderPassActive;
    }

private:

    bool mIsRenderPassActive = false;

};

}
