#pragma once

#include "RenderPass.hpp"

namespace MFA {

class DisplayRenderPass;

namespace RF = RenderFrontend;
namespace RB = RenderBackend;

class ShadowRenderPassV2 final : public RenderPass {
public:
    
    explicit ShadowRenderPassV2(
        uint32_t const imageWidth = 2048,
        uint32_t const imageHeight = 2048
    )
        : mImageWidth(imageWidth)
        , mImageHeight(imageHeight)
    {}
    ~ShadowRenderPassV2() override = default;

    ShadowRenderPassV2 (ShadowRenderPassV2 const &) noexcept = delete;
    ShadowRenderPassV2 (ShadowRenderPassV2 &&) noexcept = delete;
    ShadowRenderPassV2 & operator = (ShadowRenderPassV2 const &) noexcept = delete;
    ShadowRenderPassV2 & operator = (ShadowRenderPassV2 &&) noexcept = delete;

    VkRenderPass GetVkRenderPass() override;

    [[nodiscard]]
    RB::DepthImageGroup const & GetDepthCubeMap() const;

    void PrepareCubemapForTransferDestination(RF::DrawPass const & drawPass);

    void PrepareCubemapForSampling(RF::DrawPass const & drawPass);

    void SetNextPassParams(int faceIndex);

protected:

    void internalInit() override;

    void internalShutdown() override;

    void internalBeginRenderPass(RF::DrawPass const & drawPass) override;

    void internalEndRenderPass(RF::DrawPass const & drawPass) override;

    void internalResize() override;

private:

    void createRenderPass();

    void createFrameBuffer(VkExtent2D const & shadowExtent);

    inline static constexpr VkFormat SHADOW_MAP_FORMAT = VK_FORMAT_R32_SFLOAT;

    VkRenderPass mVkRenderPass {};
    VkFramebuffer mFrameBuffer {};
    
    RB::DepthImageGroup mDepthCubeMap {};
    RB::DepthImageGroup mDepthImage {};

    uint32_t mImageWidth = 0;
    uint32_t mImageHeight = 0;

    DisplayRenderPass * mDisplayRenderPass = nullptr;

    int mFaceIndex = -1;
};

}
