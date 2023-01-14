#pragma once

namespace MFA
{
    class RenderResource
    {
    public:

        explicit RenderResource();

        virtual ~RenderResource();

    protected:

        virtual void OnResize() = 0;

    private:

        int mResizeEventId = -1;

    };
}
