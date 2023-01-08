#pragma once

#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/render_resources/RenderResources.hpp"

namespace MFA
{
    class DirectionalLightShadowResources : RenderResources
    {
    public:

        void Init(VkRenderPass renderPass);

        void Shutdown();

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

        void CreateShadowMap(VkExtent2D const & shadowExtent);

        std::vector<std::shared_ptr<RT::DepthImageGroup>> mShadowMaps{};

    };
}
