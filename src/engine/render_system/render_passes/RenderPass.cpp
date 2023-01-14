#include "RenderPass.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {


    //-------------------------------------------------------------------------------------------------

    RenderPass::RenderPass()
    {
        mResizeEventId = RF::AddResizeEventListener2([this]()->void
        {
            OnResize();
        });
    }

    //-------------------------------------------------------------------------------------------------

    RenderPass::~RenderPass()
    {
        RF::RemoveResizeEventListener2(mResizeEventId);
    }

    //-------------------------------------------------------------------------------------------------

    void RenderPass::BeginRenderPass(RT::CommandRecordState & recordState)
    {
        MFA_ASSERT(mIsRenderPassActive == false);
        mIsRenderPassActive = true;
        recordState.renderPass = this;
    }

    //-------------------------------------------------------------------------------------------------

    void RenderPass::EndRenderPass(RT::CommandRecordState & recordState)
    {
        MFA_ASSERT(mIsRenderPassActive == true);
        recordState.renderPass = nullptr;
        mIsRenderPassActive = false;
    }

    //-------------------------------------------------------------------------------------------------

}
