#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "engine/render_system/RenderTypes.hpp"
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>
#else
#include <vulkan/vulkan.h>
#endif

namespace MFA {

class RenderPass {

public:

    virtual ~RenderPass() = default;

    void Init();

    void Shutdown();

    virtual void OnResize();

    virtual VkRenderPass GetVkRenderPass() = 0;

protected:

    virtual void BeginRenderPass(RT::CommandRecordState & drawPass);

    virtual void EndRenderPass(RT::CommandRecordState & drawPass);

    virtual void internalInit() = 0;

    virtual void internalShutdown() = 0;
    
    [[nodiscard]]
    bool getIsRenderPassActive() const {
        return mIsRenderPassActive;
    }

private:

    bool mIsRenderPassActive = false;

};

}
