#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_resources/RenderResource.hpp"

namespace MFA
{
    class DirectionalLightShadowResource : public RenderResource
    {
    public:

        explicit DirectionalLightShadowResource(
            uint32_t width = RT::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH,
            uint32_t height = RT::DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT
        );

        ~DirectionalLightShadowResource();

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowMap(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        RT::DepthImageGroup const & GetShadowMap(uint32_t frameIndex) const;

        // Appends required data for barrier to execute
        static void PrepareForSampling(
            RT::CommandRecordState const & recordState,
            bool isUsed,
            std::vector<VkImageMemoryBarrier> & outPipelineBarriers
        );

        void OnResize() override;

    private:

        static constexpr uint32_t CubeFaceCount = 6;

        void CreateShadowMap();

        std::vector<std::shared_ptr<RT::DepthImageGroup>> mShadowMaps{};
        VkExtent2D mShadowExtend {};

    };
}
