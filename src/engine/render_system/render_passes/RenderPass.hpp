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

        virtual VkRenderPass GetVkRenderPass() = 0;

        virtual void BeginRenderPass(RT::CommandRecordState & recordState);

        virtual void EndRenderPass(RT::CommandRecordState & recordState);

    protected:

        [[nodiscard]]
        bool getIsRenderPassActive() const {
            return mIsRenderPassActive;
        }

    private:

        bool mIsRenderPassActive = false;

    };

}
