#pragma once

#include "engine/render_system/render_resources/RenderResource.hpp"
#include "engine/render_system/RenderTypes.hpp"

#include <memory>
#include <vector>

namespace MFA
{
    class MSSAA_RenderResource final : public RenderResource
    {
    public:

        explicit MSSAA_RenderResource();

        ~MSSAA_RenderResource() override;

        MSSAA_RenderResource(MSSAA_RenderResource const &) noexcept = delete;
        MSSAA_RenderResource(MSSAA_RenderResource &&) noexcept = delete;
        MSSAA_RenderResource & operator = (MSSAA_RenderResource const &) noexcept = delete;
        MSSAA_RenderResource & operator = (MSSAA_RenderResource &&) noexcept = delete;

        [[nodiscard]]
        RT::ColorImageGroup const & GetImageGroup(int index);

    protected:

        void OnResize() override;

    private:

        void CreateMSAAImage();

        std::vector<std::shared_ptr<RT::ColorImageGroup>> mMSAAImageGroupList{};

    };
}
