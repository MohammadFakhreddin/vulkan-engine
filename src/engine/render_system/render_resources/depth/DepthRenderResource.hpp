#pragma once

#include "engine/render_system/render_resources/RenderResource.hpp"
#include "engine/render_system/RenderTypes.hpp"

#include <memory>
#include <vector>

namespace MFA
{
    class DepthRenderResource final : public RenderResource
    {
    public:

        explicit DepthRenderResource();

        ~DepthRenderResource() override;

        DepthRenderResource(DepthRenderResource const &) noexcept = delete;
        DepthRenderResource(DepthRenderResource &&) noexcept = delete;
        DepthRenderResource & operator = (DepthRenderResource const &) noexcept = delete;
        DepthRenderResource & operator = (DepthRenderResource &&) noexcept = delete;

        [[nodiscard]]
        RT::DepthImageGroup const & GetDepthImage(int index) const;

        void CreateDepthImages();

    protected:

        void OnResize() override;

    private:

        std::vector<std::shared_ptr<RT::DepthImageGroup>> mDepthImageGroupList{};

    };
}
