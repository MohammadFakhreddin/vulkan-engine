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

    // Render pass has fixed resources. Change in resource causes change in frame-buffer
    class RenderPass {

    public:

        explicit RenderPass();

        virtual ~RenderPass();

        virtual VkRenderPass GetVkRenderPass() = 0;

        virtual void BeginRenderPass(RT::CommandRecordState & recordState);

        virtual void EndRenderPass(RT::CommandRecordState & recordState);

        virtual void OnResize() = 0;

    protected:

        [[nodiscard]]
        bool getIsRenderPassActive() const {
            return mIsRenderPassActive;
        }

    private:

        bool mIsRenderPassActive = false;

        int mResizeEventId = -1;

    };

}
