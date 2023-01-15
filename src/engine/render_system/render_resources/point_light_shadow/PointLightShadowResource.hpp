#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_resources/RenderResource.hpp"

#include <vector>

namespace MFA
{

    class PointLightShadowResource final : public RenderResource
    {
    public:

        explicit PointLightShadowResource(
            uint32_t width = RT::POINT_LIGHT_SHADOW_WIDTH,
            uint32_t height = RT::POINT_LIGHT_SHADOW_HEIGHT
        );

        ~PointLightShadowResource() override;

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowCubeMap(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowCubeMap(uint32_t frameIndex) const;

        void OnResize() override;

        [[nodiscard]]
        VkExtent2D GetShadowExtent() const;

    private:

        static constexpr uint32_t CubeFaceCount = 6;

        void CreateShadowCubeMap();
        
        std::vector<std::shared_ptr<RT::DepthImageGroup>> mShadowCubeMapList{};

        VkExtent2D mShadowExtent{};
    };

}
