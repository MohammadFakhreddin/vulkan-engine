#include "RenderResource.hpp"

#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    RenderResource::RenderResource()
    {
        mResizeEventId = RF::AddResizeEventListener1([this]()->void
        {
            OnResize();
        });
    }

    //-------------------------------------------------------------------------------------------------

    RenderResource::~RenderResource()
    {
        RF::RemoveResizeEventListener1(mResizeEventId);
    }

    //-------------------------------------------------------------------------------------------------

}
